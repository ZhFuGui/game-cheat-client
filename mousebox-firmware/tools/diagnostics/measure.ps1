param([ValidateRange(1,60)] [int]$Seconds=30, [string]$DeviceMatch='VID_046D&PID_C54D&MI_00', [string]$LogPath='', [switch]$Foreground)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Windows.Forms,System.Windows.Forms.Primitives
$taskReferences=@(Get-ChildItem -LiteralPath (Join-Path $PSHOME 'ref') -Filter '*.dll' | ForEach-Object { $_.FullName }) +
    @([System.Windows.Forms.NativeWindow].Assembly.Location,[System.Windows.Forms.Message].Assembly.Location)
Add-Type -Path (Join-Path $PSScriptRoot 'RawInputMonitor.cs') -ReferencedAssemblies $taskReferences
$taskMonitor=[MouseboxRawInputMonitor]::new($DeviceMatch,[bool]$Foreground)
try { $taskMonitor.Run($Seconds,$LogPath) } finally { $taskMonitor.Dispose() }
