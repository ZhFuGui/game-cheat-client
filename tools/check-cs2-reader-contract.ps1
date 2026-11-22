$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib/files.ps1')
$repoRoot = Split-Path $PSScriptRoot -Parent
$contractRoot = Join-Path $repoRoot 'protocol/sources/cs2_reader'
$manifest = Get-Content -LiteralPath (Join-Path $contractRoot 'manifest.json') -Raw | ConvertFrom-Json
if ($manifest.protocol -ne 'cs2.reader.v2' -or $manifest.protocol_version -ne 2 -or
    $manifest.schema_sha256 -ne 'de589b63c5efe26492a267bafed709034f8ad31cec02990a3662a87eacaff727') {
    throw 'CS2 descriptor is not the reviewed v2 contract'
}
foreach ($entry in $manifest.files) {
    $target = Join-Path $contractRoot $entry.path
    if ((Get-ContentDigest $target).ToLowerInvariant() -ne $entry.sha256) { throw "CS2 frozen contract changed: $($entry.path)" }
}
if ((Get-ContentDigest (Join-Path $contractRoot 'reader-v2.descriptor.pb')).ToLowerInvariant() -ne $manifest.schema_sha256) { throw 'CS2 descriptor digest mismatch' }
$schema = Get-Content -LiteralPath (Join-Path $repoRoot 'third_party/cs2-reader/include/cs2_reader/v2/schema.hpp') -Raw
$digestBytes = ([regex]::Matches($schema, 'std::byte\{0x([0-9a-f]{2})\}') | ForEach-Object { $_.Groups[1].Value }) -join ''
if ($digestBytes -ne $manifest.schema_sha256 -or !$schema.Contains($manifest.schema_sha256) -or
    !$schema.Contains('protocol_version = 2;')) { throw 'CS2 SDK handshake does not match frozen contract' }
$reference = $manifest.recoil_evaluator
if ($reference.path -ne 'third_party/cs2-reader/include/cs2_reader/v2/recoil.hpp' -or $reference.sha256 -ne 'c657fc60db1e0ed953904b1f3cc0f8e9e10b36e1ae56a996e5fa567cf75cbb3f') { throw 'CS2 recoil evaluator is not the reviewed reference' }
if ((Get-ContentDigest (Join-Path $repoRoot $reference.path)).ToLowerInvariant() -ne $reference.sha256) { throw 'CS2 frozen recoil evaluator changed' }
foreach ($entry in $manifest.sdk_files) {
    if ((Get-ContentDigest (Join-Path $repoRoot $entry.path)).ToLowerInvariant() -ne $entry.sha256) { throw "CS2 frozen SDK changed: $($entry.path)" }
}
Write-Output 'PASS: reviewed CS2 Reader v2 / 3 proto / frozen descriptor / SDK'
