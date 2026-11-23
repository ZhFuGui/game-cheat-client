$ErrorActionPreference = 'Stop'
# One process/entry for all frozen boundaries, including direct Visual Studio builds.
& (Join-Path $PSScriptRoot 'check-reader-contract.ps1')
& (Join-Path $PSScriptRoot 'check-cs2-reader-contract.ps1')
& (Join-Path $PSScriptRoot 'check-valorant-reader-contract.ps1')
& (Join-Path $PSScriptRoot 'check-traceline-package.ps1')
& (Join-Path $PSScriptRoot 'check-font-resources.ps1')
