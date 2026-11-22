function ConvertTo-FirmwareSerialPort {
    param([Parameter(Mandatory=$true)]$Device)
    if ($Device.FriendlyName -notmatch '\((COM[0-9]+)\)\s*$') { return }
    [pscustomobject]@{ DeviceID=$Matches[1]; Name=$Device.FriendlyName; PNPDeviceID=$Device.InstanceId }
}

function Get-FirmwareSerialPorts {
    # Win32_SerialPort may return after Caterina's entire ~8 second window.
    # PnP present devices are the current source, not a cached historical list.
    foreach ($device in @(Get-PnpDevice -Class Ports -PresentOnly -ErrorAction Stop)) {
        ConvertTo-FirmwareSerialPort $device
    }
}

function ConvertTo-BoardUsbLocation {
    param([string[]]$LocationPaths)
    # A composite interface adds USBMI; its bootloader uses the same physical
    # route without that suffix. Prefer this route over alternate device paths.
    $taskPaths = @($LocationPaths | Where-Object { $_ -match '^PCIROOT\([^\r\n]+#USBROOT\(\d+\)#USB\(\d+\)(#USB\(\d+\))*(#USBMI\(\d+\))?$' })
    if (!$taskPaths.Count) { throw 'No verifiable PCI/USB physical location.' }
    $taskInterfaces = @($taskPaths | Where-Object { $_ -match '#USBMI\(\d+\)$' })
    $taskSelected = if ($taskInterfaces.Count) { $taskInterfaces[0] } else { $taskPaths[0] }
    return ($taskSelected -replace '#USBMI\(\d+\)$', '').ToUpperInvariant()
}

function Assert-BoardUsbLocation {
    param([string]$InstanceId, [string]$ExpectedLocationPath)
    if (!$ExpectedLocationPath) { return }
    $taskExpectedLocation = ConvertTo-BoardUsbLocation @($ExpectedLocationPath)
    $taskProperty = Get-PnpDeviceProperty -InstanceId $InstanceId -KeyName DEVPKEY_Device_LocationPaths -ErrorAction Stop
    $taskActualLocation = ConvertTo-BoardUsbLocation @($taskProperty.Data)
    if ($taskActualLocation -cne $taskExpectedLocation) {
        throw "Refusing another board: USB location $taskActualLocation differs from $taskExpectedLocation."
    }
}

function Select-BoardBootloaders {
    param([object[]]$Devices, [string]$ExpectedInstanceId = '')
    @($Devices | Where-Object {
        $_.PNPDeviceID -match '^USB\\VID_(2341|2A03)&PID_003[67]\\' -and
        (!$ExpectedInstanceId -or $_.PNPDeviceID -eq $ExpectedInstanceId)
    })
}
