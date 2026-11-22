param([string]$Root, $Target, [string]$ArduinoCliPath, [string]$BuildName)
$ErrorActionPreference = 'Stop'
. (Join-Path $Root 'tools/toolchain.ps1')
. (Join-Path $Root 'tools/common/target.ps1')
$lock = Assert-FirmwareDependencies $Root
$base = Resolve-FirmwarePath $Root ('build/' + $Target.id + '/' + $BuildName)
$stage = Join-Path $base 'work'
$null = New-Item -ItemType Directory -Path $base -Force
# Staging and object outputs are disposable; validate their absolute workspace
# paths before removal so rebuilding cannot retain obsolete patched SDK files.
foreach ($part in @('work','output')) {
    $disposable = Resolve-FirmwarePath $Root ('build/' + $Target.id + '/' + $BuildName + '/' + $part)
    if (Test-Path -LiteralPath $disposable) { Remove-Item -LiteralPath $disposable -Recurse -Force }
}
$user = Join-Path $stage 'arduino'
$platform = Join-Path $user 'hardware/mousebox/avr'
foreach ($directory in @($platform, (Join-Path $platform 'cores/bridge'), (Join-Path $user 'libraries'))) {
    $null = New-Item -ItemType Directory -Path $directory -Force
}
$upstream = Join-Path $Root 'third_party/upstream/arduino-avr-1.8.7'
Copy-Item -Path (Join-Path $upstream 'cores/arduino/*') -Destination (Join-Path $platform 'cores/bridge') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $upstream 'variants') -Destination $platform -Recurse -Force
Copy-Item -LiteralPath (Join-Path $upstream 'libraries') -Destination $platform -Recurse -Force
Copy-Item -LiteralPath (Join-Path $upstream 'platform.txt') -Destination $platform -Force
foreach ($file in $lock.overlays.files) {
    $destination = Join-Path $platform $file.path
    $null = New-Item -ItemType Directory -Path (Split-Path $destination) -Force
    Copy-Item -LiteralPath (Join-Path $Root ('third_party/' + $lock.overlays.root + '/' + $file.path)) -Destination $destination -Force
}
Copy-Item -LiteralPath (Join-Path $Root 'platforms/boards/leonardo_r3_max3421e/boards.txt') -Destination $platform -Force
$uhs = Join-Path $user 'libraries/USB_Host_Shield_Library_2.0'
$null = New-Item -ItemType Directory -Path $uhs -Force
Copy-Item -Path (Join-Path $Root 'third_party/upstream/usb-host-shield-1.7.0/*') -Destination $uhs -Recurse -Force
foreach ($library in @('MouseboxProtocol', 'MouseboxTarget')) {
    $libraryRoot = Join-Path $user ('libraries/' + $library)
    $null = New-Item -ItemType Directory -Path (Join-Path $libraryRoot 'src') -Force
    [IO.File]::WriteAllText((Join-Path $libraryRoot 'library.properties'), "name=$library`nversion=$($Target.version)`nauthor=Mousebox`nmaintainer=Mousebox`nsentence=Mousebox build input`nparagraph=Generated from canonical sources`ncategory=Other`narchitectures=avr`n", [Text.UTF8Encoding]::new($false))
}
$protocol = Join-Path $user 'libraries/MouseboxProtocol/src'
Copy-Item -Path (Join-Path $Root 'protocol/include/*') -Destination $protocol -Force
$targetInclude = Join-Path $user 'libraries/MouseboxTarget/src'
Write-FirmwareTargetHeader $Root $Target $targetInclude
$composition = (Resolve-FirmwarePath $Root $Target.composition).Replace('\', '/')
[IO.File]::WriteAllText((Join-Path $targetInclude 'MouseboxTarget.h'), "#pragma once`n#include `"$composition`"`n", [Text.UTF8Encoding]::new($false))
$sketch = Join-Path $stage 'relay'
$null = New-Item -ItemType Directory -Path $sketch -Force
Copy-Item -LiteralPath (Join-Path $Root 'apps/relay/relay.ino') -Destination $sketch -Force
$cli = Resolve-ArduinoCli $ArduinoCliPath
$previous = [Environment]::GetEnvironmentVariable('ARDUINO_DIRECTORIES_USER', 'Process')
try {
    [Environment]::SetEnvironmentVariable('ARDUINO_DIRECTORIES_USER', $user, 'Process')
    # The IDE-bundled CLI can crash in its optional version subcommand without
    # an IDE configuration. Identify this exact compiler by SHA and file metadata.
    $toolVersion = [Diagnostics.FileVersionInfo]::GetVersionInfo($cli).ProductVersion
    if (!$toolVersion) { $toolVersion = 'unavailable-file-metadata; see arduinoCliSha256' }
    $ErrorActionPreference = 'Continue'
    & $cli compile --fqbn $Target.fqbn --warnings all --build-path (Join-Path $base 'output') $sketch 2>&1 |
        Tee-Object -FilePath (Join-Path $base 'compile.log') | Write-Host
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if ($code -ne 0) { throw 'Firmware compilation failed.' }
} finally { [Environment]::SetEnvironmentVariable('ARDUINO_DIRECTORIES_USER', $previous, 'Process') }
$inputs = foreach ($part in @('apps','core','protocol','profiles','platforms','targets','tools')) {
    Get-ChildItem -LiteralPath (Join-Path $Root $part) -File -Recurse | Where-Object { $_.Extension -in '.hpp','.h','.ino','.cpp','.cs','.ps1','.json','.txt' } |
        ForEach-Object { [ordered]@{ path=$_.FullName.Substring($Root.Length + 1).Replace('\','/'); sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash } }
}
$hex = Join-Path $base 'output/relay.ino.hex'
$manifest = [ordered]@{ schemaVersion=1; target=$Target.id; firmwareVersion=$Target.version; protocolVersion=1;
    capabilities=$Target.capabilities; flash=$Target.flash; artifact='output/relay.ino.hex';
    sha256=(Get-FileHash -LiteralPath $hex -Algorithm SHA256).Hash; diagnosticLogging=$false; deployed=$false;
    dependenciesLockSha256=(Get-FileHash -LiteralPath (Join-Path $Root 'third_party/dependencies.lock.json') -Algorithm SHA256).Hash;
    toolchain=@{ arduinoCliSha256=(Get-FileHash -LiteralPath $cli -Algorithm SHA256).Hash; arduinoCliVersion=$toolVersion };
    sourceFiles=@($inputs) }
$manifest | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $base 'manifest.json') -Encoding UTF8
Write-Output ('BUILD_MANIFEST=' + (Join-Path $base 'manifest.json'))
