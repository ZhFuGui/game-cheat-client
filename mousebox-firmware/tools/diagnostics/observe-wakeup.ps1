param(
    [ValidateRange(1,60)][int]$Seconds=40,
    [ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$Label='local',
    [string]$OutputDirectory='',
    [switch]$ValidateOnly
)
$ErrorActionPreference='Stop'
if ($PSVersionTable.PSEdition -ne 'Core') { throw 'Run this background observer with PowerShell 7 (pwsh).' }
$taskRoot=(Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).Path
$taskNative=@(Get-CimInstance Win32_SerialPort | Where-Object {
    $_.PNPDeviceID -match '^USB\\VID_046D&PID_C54D&MI_03\\'
})
if ($taskNative.Count -ne 1) { throw 'Expected exactly one native box on this PC; no device opened.' }
$taskInstance=$taskNative[0].PNPDeviceID
$taskParent=(Get-PnpDeviceProperty -InstanceId $taskInstance -KeyName 'DEVPKEY_Device_Parent').Data
$taskChildren=@((Get-PnpDeviceProperty -InstanceId $taskParent -KeyName 'DEVPKEY_Device_Children').Data)
$taskMouse=@($taskChildren | Where-Object { $_ -match '^USB\\VID_046D&PID_C54D&MI_00\\' })
if ($taskMouse.Count -ne 1) { throw 'Expected exactly one mouse interface below the native box.' }
$taskHid=@((Get-PnpDeviceProperty -InstanceId $taskMouse[0] -KeyName 'DEVPKEY_Device_Children').Data | Where-Object { $_ -like 'HID\VID_046D&PID_C54D&MI_00\*' })
$taskHub=(Get-PnpDeviceProperty -InstanceId $taskParent -KeyName 'DEVPKEY_Device_Parent').Data
[string[]]$taskInstances=@(@($taskParent,$taskMouse[0],$taskInstance,$taskHub)+$taskHid | Select-Object -Unique)

Add-Type -AssemblyName System.Windows.Forms,System.Windows.Forms.Primitives
$taskReferences=@(Get-ChildItem -LiteralPath (Join-Path $PSHOME 'ref') -Filter '*.dll' | ForEach-Object { $_.FullName }) +
    @([System.Windows.Forms.NativeWindow].Assembly.Location,[System.Windows.Forms.Message].Assembly.Location) +
    @(Get-ChildItem -LiteralPath $PSHOME -Filter 'System.Private.Windows*.dll' | ForEach-Object { $_.FullName })
Add-Type -Path @((Join-Path $PSScriptRoot 'RawInputMonitor.cs'),(Join-Path $PSScriptRoot 'WakeupObservation.cs')) -ReferencedAssemblies $taskReferences
if ($ValidateOnly) { [MouseboxWakeupObservation]::Validate($taskInstances); return }
if (!$OutputDirectory) { $OutputDirectory=Join-Path $taskRoot ('build\wakeup-' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
if (!(Test-Path -LiteralPath $OutputDirectory)) { New-Item -ItemType Directory -Path $OutputDirectory | Out-Null }
$taskOutput=(Resolve-Path -LiteralPath $OutputDirectory).Path
$taskPrefix=Join-Path $taskOutput $Label
foreach ($taskSuffix in @('-raw.csv','-power.csv','-metadata.log','-summary.log')) {
    if (Test-Path -LiteralPath ($taskPrefix+$taskSuffix)) { throw 'Observation already exists; refusing overwrite.' }
}
[MouseboxWakeupObservation]::Run($Seconds,'VID_046D&PID_C54D&MI_00',$taskInstances,$taskPrefix)
