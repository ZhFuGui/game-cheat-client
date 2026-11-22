$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib/files.ps1')
$fontRoot = Join-Path (Split-Path $PSScriptRoot -Parent) 'data/fonts'
$font = (Get-Content -LiteralPath (Join-Path $fontRoot 'manifest.json') -Raw -Encoding UTF8 | ConvertFrom-Json).primary
if ($font.file -notmatch '^[a-z0-9-]+\.ttf$' -or $font.sha256 -notmatch '^[a-f0-9]{64}$') {
    throw 'Primary font identity is invalid'
}
$fontPath = Join-Path $fontRoot $font.file
if ((Get-Item -LiteralPath $fontPath).Length -ne $font.bytes -or (Get-ContentDigest $fontPath) -ine $font.sha256) {
    throw 'Primary font differs from its reviewed resource manifest'
}
Write-Output "PASS: bundled primary font $($font.postscriptName) matches its source digest"
