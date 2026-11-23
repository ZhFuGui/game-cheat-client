$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib/files.ps1')
$contractRoot = Join-Path (Split-Path $PSScriptRoot -Parent) 'protocol/sources/valorant_reader'
$lock = Get-Content -LiteralPath (Join-Path $contractRoot 'contract-lock.json') -Raw | ConvertFrom-Json
if ($lock.protocol -ne 'valorant.reader.v1') { throw 'Unexpected VALORANT protocol lock' }
foreach ($entry in $lock.files) {
    $actual = (Get-ContentDigest (Join-Path $contractRoot $entry.path)).ToLowerInvariant()
    if ($actual -ne $entry.sha256) { throw "VALORANT frozen input mismatch: $($entry.path)" }
}
$manifest = Get-Content -LiteralPath (Join-Path $contractRoot 'reader/v1/schema-manifest.json') -Raw | ConvertFrom-Json
if (-not $manifest.frozen -or $manifest.descriptor_sha256 -ne $lock.descriptorSha256) {
    throw 'VALORANT manifest and locked descriptor disagree'
}
$codec = Get-Content -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) 'src/adapters/sources/valorant_reader/codec.hpp') -Raw
if (-not $codec.Contains($lock.descriptorSha256)) { throw 'VALORANT consumer handshake digest is stale' }
Write-Output 'PASS: frozen VALORANT Reader v1 / 3 independent streams / descriptor and source bytes pinned'
