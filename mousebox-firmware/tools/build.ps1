param(
    [Parameter(Mandatory=$true)][string]$Target,
    [string]$ArduinoCliPath = '',
    [ValidateSet('relay','simulation')][string]$Product = 'relay',
    [string]$BuildName = 'release-v7'
)
. (Join-Path $PSScriptRoot 'toolchain.ps1')
. (Join-Path $PSScriptRoot 'common/target.ps1')
if ($BuildName -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Invalid build name.' }
$taskTarget = Read-FirmwareTarget $taskRoot $Target
if ($Product -eq 'simulation' -and $taskTarget.builder -eq 'simulation') {
    & (Join-Path $PSScriptRoot 'test.ps1') -Suite simulation
} elseif ($Product -eq 'relay' -and $taskTarget.builder -eq 'arduino-avr') {
    & (Join-Path $PSScriptRoot 'builders/arduino-avr.ps1') -Root $taskRoot -Target $taskTarget -ArduinoCliPath $ArduinoCliPath -BuildName $BuildName
} else {
    throw 'This target/product combination is not implemented.'
}
