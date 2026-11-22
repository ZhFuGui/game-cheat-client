param(
    [Parameter(Mandatory=$true)][ValidateSet('List','Backup','BackupAndUpload')][string]$Action,
    [string]$Target = '',
    [string]$RuntimePort = '',
    [string]$ExpectedLocationPath = '',
    [string]$ArtifactManifest = '',
    [string]$BackupName = '',
    [switch]$ManualReset,
    [ValidateRange(15,300)][int]$ResetWaitSeconds = 60
)
. (Join-Path $PSScriptRoot 'toolchain.ps1')
. (Join-Path $PSScriptRoot 'device-location.ps1')
. (Join-Path $PSScriptRoot 'common/target.ps1')
. (Join-Path $PSScriptRoot 'common/artifact.ps1')
if ($Action -eq 'List') {
    Get-FirmwareSerialPorts | Select-Object DeviceID,Name,PNPDeviceID
    return
}
if (!$RuntimePort -or !$ExpectedLocationPath -or !$Target) { throw 'Explicit target, runtime port and physical USB location are required.' }
$taskTarget = Read-FirmwareTarget $taskRoot $Target
if ($taskTarget.builder -ne 'arduino-avr') { throw 'This target has no implemented flash backend.' }
$ExpectedLocationPath = ConvertTo-BoardUsbLocation @($ExpectedLocationPath)
if (!$BackupName) { $BackupName = 'before-'+$Target+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss') }
if ($BackupName -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Invalid backup name.' }
$taskArtifact = $null
if ($Action -eq 'BackupAndUpload') {
    if (!$ArtifactManifest) { throw 'An explicit validated release manifest is required.' }
    $taskArtifact = Assert-FirmwareArtifact $taskRoot $taskTarget $ArtifactManifest
}
$taskBackup = Resolve-FirmwarePath $taskRoot ('backups/'+$BackupName)
if (Test-Path -LiteralPath $taskBackup) { throw 'Backup already exists; refusing to overwrite.' }

function Assert-RuntimeDevice {
    $identity = '^USB\\VID_'+$taskTarget.flash.runtimeVid+'&PID_'+$taskTarget.flash.runtimePid+'&MI_'+$taskTarget.flash.runtimeInterface+'\\'
    $devices = @(Get-FirmwareSerialPorts | Where-Object { $_.DeviceID -eq $RuntimePort -and $_.PNPDeviceID -match $identity })
    if ($devices.Count -ne 1) { throw 'Expected runtime port/USB identity is missing.' }
    Assert-BoardUsbLocation $devices[0].PNPDeviceID $ExpectedLocationPath
    return $devices[0]
}
function Enter-TargetBootloader {
    $null = Assert-RuntimeDevice
    if (@(Select-BoardBootloaders @(Get-FirmwareSerialPorts)).Count) { throw 'An existing bootloader makes the target ambiguous.' }
    if (!$ManualReset) {
        $serial = [IO.Ports.SerialPort]::new($RuntimePort,1200)
        try { $serial.DtrEnable=$true; $serial.Open(); $serial.DtrEnable=$false } finally { $serial.Dispose() }
    }
    $waitMs = if ($ManualReset) { $ResetWaitSeconds*1000 } else { 8000 }
    $watch = [Diagnostics.Stopwatch]::StartNew()
    while ($watch.ElapsedMilliseconds -lt $waitMs) {
        $devices = @(Select-BoardBootloaders @(Get-FirmwareSerialPorts))
        $matching = @($devices | Where-Object {
            $property = Get-PnpDeviceProperty -InstanceId $_.PNPDeviceID -KeyName DEVPKEY_Device_LocationPaths -ErrorAction Stop
            (ConvertTo-BoardUsbLocation @($property.Data)) -ceq $ExpectedLocationPath
        })
        if ($matching.Count -gt 1) { throw 'Multiple matching bootloaders.' }
        if ($matching.Count -eq 1) { return $matching[0].DeviceID }
        Start-Sleep -Milliseconds 150
    }
    throw 'Target bootloader not found; no flash operation performed.'
}
$device = Assert-RuntimeDevice
$tool = Resolve-Avrdude
$null = New-Item -ItemType Directory -Path $taskBackup
$receiptPath = Join-Path $taskBackup 'operation.json'
$receipt = [ordered]@{ target=$Target; action=$Action; started=(Get-Date -Format o); runtimePort=$RuntimePort;
    runtimeInstance=$device.PNPDeviceID; location=$ExpectedLocationPath; success=$false; artifactSha256=$null;
    firmwareVersion=$null; readback=$null; error=$null; bootPort=$null; finished=$null;
    sourceOfPriorFirmware='Not inferred from the current working tree.' }
if ($taskArtifact) {
    $receipt.artifactSha256=$taskArtifact.Manifest.sha256
    $receipt.firmwareVersion=$taskArtifact.Manifest.firmwareVersion
    Copy-Item -LiteralPath $taskArtifact.ManifestPath -Destination (Join-Path $taskBackup 'requested-artifact.json')
}
try {
    $receipt | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $receiptPath -Encoding UTF8
    $boot = Enter-TargetBootloader
    $receipt.bootPort=$boot
    $arguments = @('-C',$tool.Config,'-p',$taskTarget.flash.mcu,'-c',$taskTarget.flash.backend,'-P',$boot,
        '-b',([string]$taskTarget.flash.bootBaudRate),'-A','-D',
        '-U',('flash:r:'+(Join-Path $taskBackup 'flash-before.bin')+':r'),
        '-U',('eeprom:r:'+(Join-Path $taskBackup 'eeprom-before.bin')+':r'),
        '-U',('flash:v:'+(Join-Path $taskBackup 'flash-before.bin')+':r'))
    if ($taskArtifact) {
        # One programmer process: verify the backup before writing, automatic write
        # verification, then independent full Flash/EEPROM readback for the receipt.
        $arguments += @('-U',('flash:w:'+$taskArtifact.Hex+':i'),
            '-U',('flash:r:'+(Join-Path $taskBackup 'flash-after.bin')+':r'),
            '-U',('eeprom:r:'+(Join-Path $taskBackup 'eeprom-after.bin')+':r'))
    } else { $arguments = @('-n')+$arguments }
    Invoke-AvrdudeBounded -Tool $tool -Arguments $arguments -LogName ('v7-'+$Action)
    if ($taskArtifact) { $receipt.readback=Assert-FirmwareReadback $taskTarget $taskArtifact $taskBackup }
    $receipt.success=$true
} catch { $receipt.error=$_.Exception.Message; throw }
finally {
    $receipt.finished=(Get-Date -Format o)
    $receipt | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $receiptPath -Encoding UTF8
    Get-ChildItem -LiteralPath $taskBackup -Filter '*.bin' | Get-FileHash -Algorithm SHA256 |
        Select-Object Path,Hash | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $taskBackup 'hashes.json') -Encoding UTF8
}
Write-Output ('DEVICE_RECEIPT='+$receiptPath)
