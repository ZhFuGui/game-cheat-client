param(
    [Parameter(Mandatory=$true)][string]$Target,
    [Parameter(Mandatory=$true)][string]$BuildManifest,
    [Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$ReleaseName
)
. (Join-Path $PSScriptRoot 'toolchain.ps1')
. (Join-Path $PSScriptRoot 'common/target.ps1')
. (Join-Path $PSScriptRoot 'common/artifact.ps1')
$targetInfo = Read-FirmwareTarget $taskRoot $Target
$artifact = Assert-FirmwareArtifact $taskRoot $targetInfo $BuildManifest
$null = Assert-FirmwareDependencies $taskRoot
$release = Resolve-FirmwarePath $taskRoot ('releases/'+$ReleaseName)
if (Test-Path -LiteralPath $release) { throw 'Release exists; refusing to replace fixed artifacts.' }
# Reject a stale build before taking its source snapshot.
foreach ($file in $artifact.Manifest.sourceFiles) {
    $source = Resolve-FirmwarePath $taskRoot $file.path
    if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash -ine $file.sha256) { throw ('Build is stale: '+$file.path) }
}
if ((Get-FileHash -LiteralPath (Join-Path $taskRoot 'third_party/dependencies.lock.json')).Hash -ine $artifact.Manifest.dependenciesLockSha256) { throw 'Dependencies changed since build.' }
$null = New-Item -ItemType Directory -Path $release
Copy-Item -LiteralPath $artifact.Hex -Destination (Join-Path $release 'firmware.hex')
Copy-Item -LiteralPath ([IO.Path]::ChangeExtension($artifact.Hex,'elf')) -Destination (Join-Path $release 'firmware.elf')
Copy-Item -LiteralPath (Join-Path (Split-Path $artifact.ManifestPath) 'compile.log') -Destination (Join-Path $release 'compile.log')
foreach ($file in $artifact.Manifest.sourceFiles) {
    $destination = Join-Path $release ('source/'+$file.path)
    $null = New-Item -ItemType Directory -Path (Split-Path $destination) -Force
    Copy-Item -LiteralPath (Resolve-FirmwarePath $taskRoot $file.path) -Destination $destination
}
foreach ($directory in @('third_party','tests')) { Copy-Item -LiteralPath (Join-Path $taskRoot $directory) -Destination (Join-Path $release 'source') -Recurse }
# Offline tests deliberately retain their original reference image and descriptor
# capture. Include those immutable fixtures so the source snapshot is testable.
foreach ($relative in @('releases/native-v6-500hz-20261019/logitech_native_bridge.upload-pages.hex','logs/receiver-descriptors.log')) {
    $destination = Join-Path $release ('source/'+$relative)
    $null = New-Item -ItemType Directory -Path (Split-Path $destination) -Force
    Copy-Item -LiteralPath (Resolve-FirmwarePath $taskRoot $relative) -Destination $destination
}
$artifact.Manifest.artifact='firmware.hex'
$artifact.Manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $release 'manifest.json') -Encoding UTF8
Get-ChildItem -LiteralPath (Join-Path $release 'source') -Recurse -File | ForEach-Object {
    [ordered]@{path=$_.FullName.Substring($release.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $release 'source-files.json') -Encoding UTF8
Write-Output ('RELEASE_MANIFEST='+ (Join-Path $release 'manifest.json'))
