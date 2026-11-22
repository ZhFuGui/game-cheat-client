function Read-FirmwareImage {
    param([string]$Path, [int]$ApplicationBytes)
    $bytes = [byte[]]::new($ApplicationBytes)
    $present = [bool[]]::new($ApplicationBytes)
    $base = 0; $end = 0; $eof = $false; $counted = 0
    foreach ($line in Get-Content -LiteralPath $Path) {
        if (!$line) { continue }
        if ($eof -or $line -notmatch '^:[0-9a-fA-F]+$' -or $line.Length -lt 11) { throw 'Invalid application HEX.' }
        $count = [Convert]::ToInt32($line.Substring(1,2),16)
        if ($line.Length -ne 11+2*$count) { throw 'Invalid HEX record length.' }
        $sum = 0
        for ($i=1; $i -lt $line.Length; $i+=2) { $sum += [Convert]::ToInt32($line.Substring($i,2),16) }
        if ($sum -band 255) { throw 'Invalid HEX checksum.' }
        $address = [Convert]::ToInt32($line.Substring(3,4),16)
        $kind = [Convert]::ToInt32($line.Substring(7,2),16)
        if ($kind -eq 0) {
            for ($i=0; $i -lt $count; ++$i) {
                $at = $base+$address+$i
                if ($at -lt 0 -or $at -ge $ApplicationBytes -or $present[$at]) { throw 'HEX overlaps or exceeds application region.' }
                $bytes[$at] = [Convert]::ToInt32($line.Substring(9+2*$i,2),16)
                $present[$at] = $true; ++$counted
                $end = [Math]::Max($end,$at+1)
            }
        } elseif ($kind -eq 1 -and !$count -and !$address) { $eof = $true }
        elseif ($kind -eq 4 -and $count -eq 2 -and !$address) { $base = [Convert]::ToInt32($line.Substring(9,4),16)*65536 }
        else { throw 'Unsupported application HEX record.' }
    }
    if (!$eof -or !$counted -or !$present[0] -or !$present[1]) { throw 'Incomplete application HEX.' }
    return @{ Bytes=$bytes; Present=$present; ProgramBytes=$counted; End=$end }
}

function Assert-FirmwareArtifact {
    param([string]$Root, $Target, [string]$ManifestPath)
    $full = [IO.Path]::GetFullPath($ManifestPath)
    $manifest = Get-Content -LiteralPath $full -Raw | ConvertFrom-Json
    if ($manifest.schemaVersion -ne 1 -or $manifest.target -cne $Target.id -or
        $manifest.firmwareVersion -cne $Target.version -or $manifest.protocolVersion -ne 1 -or
        $manifest.diagnosticLogging -ne $false -or $Target.builder -ne 'arduino-avr') { throw 'Artifact target/version/product mismatch.' }
    foreach ($key in @('baudRate','minIntervalMs','maxStepCounts','maxBudget','legacyFlags')) {
        if ($manifest.capabilities.$key -ne $Target.capabilities.$key) { throw ('Artifact capability mismatch: '+$key) }
    }
    foreach ($key in @('backend','mcu','flashBytes','applicationBytes','sramBytes','eepromBytes','bootBaudRate','runtimeVid','runtimePid','runtimeInterface')) {
        if ($manifest.flash.$key -cne $Target.flash.$key) { throw ('Artifact board mismatch: '+$key) }
    }
    $hex = Resolve-FirmwarePath (Split-Path $full) $manifest.artifact
    if ($manifest.sha256 -notmatch '^[0-9a-fA-F]{64}$' -or
        (Get-FileHash -LiteralPath $hex -Algorithm SHA256).Hash -ine $manifest.sha256) { throw 'Artifact SHA256 mismatch.' }
    $image = Read-FirmwareImage $hex $Target.flash.applicationBytes
    return @{ Manifest=$manifest; ManifestPath=$full; Hex=$hex; Image=$image }
}

function Assert-FirmwareReadback {
    param($Target, $Artifact, [string]$BackupDirectory)
    $before = [IO.File]::ReadAllBytes((Join-Path $BackupDirectory 'flash-before.bin'))
    $after = [IO.File]::ReadAllBytes((Join-Path $BackupDirectory 'flash-after.bin'))
    $eepromBefore = [IO.File]::ReadAllBytes((Join-Path $BackupDirectory 'eeprom-before.bin'))
    $eepromAfter = [IO.File]::ReadAllBytes((Join-Path $BackupDirectory 'eeprom-after.bin'))
    if ($before.Length -ne $Target.flash.flashBytes -or $after.Length -ne $before.Length -or
        $eepromBefore.Length -ne $Target.flash.eepromBytes -or $eepromAfter.Length -ne $eepromBefore.Length) { throw 'Incomplete backup/readback.' }
    for ($i=0; $i -lt $Artifact.Image.End; ++$i) {
        if ($Artifact.Image.Present[$i] -and $after[$i] -ne $Artifact.Image.Bytes[$i]) { throw ('Application readback mismatch at '+$i) }
    }
    for ($i=$Target.flash.applicationBytes; $i -lt $after.Length; ++$i) {
        if ($before[$i] -ne $after[$i]) { throw 'Bootloader bytes changed.' }
    }
    for ($i=0; $i -lt $eepromAfter.Length; ++$i) {
        if ($eepromBefore[$i] -ne $eepromAfter[$i]) { throw 'EEPROM bytes changed.' }
    }
    return @{ applicationBytesVerified=$Artifact.Image.ProgramBytes; bootloaderUnchanged=$true; eepromUnchanged=$true }
}
