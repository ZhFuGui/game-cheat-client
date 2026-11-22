# Recovery transport only. No fuse, EEPROM, chip-erase or bootloader writes.
function Read-Avr109Image {
    param([string]$Path)
    $bytes=[byte[]]::new(28672); $present=[bool[]]::new(28672)
    $base=0; $end=0; $eof=$false
    foreach($line in Get-Content -LiteralPath $Path) {
        if(!$line){continue}
        if($eof -or $line[0] -ne ':' -or $line.Length -lt 11){throw 'Invalid application HEX'}
        $count=[Convert]::ToInt32($line.Substring(1,2),16)
        if($line.Length -ne 11+2*$count){throw 'Invalid HEX record length'}
        $sum=0
        for($i=1;$i -lt $line.Length;$i+=2){$sum+=[Convert]::ToInt32($line.Substring($i,2),16)}
        if(($sum -band 255) -ne 0){throw 'Invalid HEX checksum'}
        $address=[Convert]::ToInt32($line.Substring(3,4),16)
        $type=[Convert]::ToInt32($line.Substring(7,2),16)
        if($type -eq 0){
            for($i=0;$i -lt $count;$i++){
                $at=$base+$address+$i
                if($at -lt 0 -or $at -ge $bytes.Length -or $present[$at]){throw 'Application range/overlap violation'}
                $present[$at]=$true; $bytes[$at]=[Convert]::ToInt32($line.Substring(9+2*$i,2),16)
                $end=[Math]::Max($end,$at+1)
            }
        } elseif($type -eq 1 -and !$count -and !$address){$eof=$true}
        elseif($type -eq 4 -and $count -eq 2){$base=[Convert]::ToInt32($line.Substring(9,4),16)*65536}
        else {throw 'Unsupported HEX record'}
    }
    if(!$eof -or !$end -or $end%128 -or ($present[0..($end-1)] -contains $false)){
        throw 'Recovery requires a contiguous, page-aligned application image'
    }
    return ,([byte[]]$bytes[0..($end-1)])
}

function Assert-Avr109Ack {
    param($Transport,[string]$Stage)
    $reply=[byte[]]$Transport.Receive(1)
    if($reply.Length -ne 1 -or $reply[0] -ne 13){throw "AVR109 acknowledgement failed: $Stage"}
}

function Set-Avr109Address {
    param($Transport,[int]$ByteAddress)
    if($ByteAddress -lt 0 -or $ByteAddress -ge 28672 -or $ByteAddress%128){throw 'Invalid application page address'}
    $word=$ByteAddress/2
    [void]$Transport.Send([byte[]]@(65,([int]$word -shr 8),([int]$word -band 255)))
    Assert-Avr109Ack $Transport 'address'
}

function Initialize-Avr109Connection {
    param($Transport,[int]$Total,[scriptblock]$Progress)
    & $Progress 'identify' 0 $Total | Out-Null
    [void]$Transport.Send([byte[]]@(83))
    if([Text.Encoding]::ASCII.GetString([byte[]]$Transport.Receive(7)) -cne 'CATERIN'){throw 'Unexpected bootloader'}
    & $Progress 'signature' 0 $Total | Out-Null
    [void]$Transport.Send([byte[]]@(115)); $signature=[byte[]]$Transport.Receive(3)
    if(($signature -join ',') -ne '135,149,30'){throw 'Unexpected MCU signature'}
    & $Progress 'capacity' 0 $Total | Out-Null
    [void]$Transport.Send([byte[]]@(98)); $capacity=[byte[]]$Transport.Receive(3)
    if(($capacity -join ',') -ne '89,0,128'){throw 'Unexpected bootloader page size'}
    & $Progress 'enter' 0 $Total | Out-Null
    [void]$Transport.Send([byte[]]@(80)); Assert-Avr109Ack $Transport 'programming mode'
}

function Get-Avr109ApplicationFacts {
    param($Transport,[byte[]]$ExpectedApplication,[scriptblock]$Progress={param($Phase,$Bytes,$Total)})
    if($ExpectedApplication.Length -ne 28672){throw 'Expected application must cover exactly the 28 KiB application area'}
    Initialize-Avr109Connection $Transport 28672 $Progress
    $facts=@{signature='1E9587';app_bytes=28672;read_only=$true}
    foreach($command in @(@{code=114;name='lock'},@{code=70;name='low_fuse'},@{code=78;name='high_fuse'},@{code=81;name='extended_fuse'})){
        [void]$Transport.Send([byte[]]@($command.code))
        $reply=[byte[]]$Transport.Receive(1)
        if($reply.Length -ne 1){throw 'Incomplete fuse observation'}
        $facts[$command.name]=$reply[0].ToString('X2')
        & $Progress $command.name 0 28672 | Out-Null
    }
    $application=[byte[]]::new(28672)
    $different=0
    for($offset=0;$offset -lt $application.Length;$offset+=128){
        Set-Avr109Address $Transport $offset
        [void]$Transport.Send([byte[]]@(103,0,128,70))
        $page=[byte[]]$Transport.Receive(128)
        if($page.Length -ne 128){throw 'Incomplete application observation'}
        [Array]::Copy($page,0,$application,$offset,128)
        for($i=0;$i -lt 128;$i++){if($page[$i] -ne $ExpectedApplication[$offset+$i]){$different++}}
        & $Progress 'read' ($offset+128) $application.Length | Out-Null
    }
    $sha=[Security.Cryptography.SHA256]::Create()
    try{
        $facts.app_sha256=([BitConverter]::ToString($sha.ComputeHash($application))).Replace('-','')
        $facts.expected_sha256=([BitConverter]::ToString($sha.ComputeHash($ExpectedApplication))).Replace('-','')
    }finally{$sha.Dispose()}
    $facts.bytes_different_from_v5=$different
    $facts.first_page_hex=([BitConverter]::ToString($application[0..127])).Replace('-','')
    [void]$Transport.Send([byte[]]@(76)); Assert-Avr109Ack $Transport 'leave read-only observation'
    [void]$Transport.Send([byte[]]@(69)); Assert-Avr109Ack $Transport 'exit read-only observation'
    return $facts
}

function Invoke-Avr109Transfer {
    param($Transport,[byte[]]$Image,[scriptblock]$Progress={param($Phase,$Bytes,$Total)})
    if(!$Image.Length -or $Image.Length -gt 28672 -or $Image.Length%128){throw 'Invalid application image size'}
    Initialize-Avr109Connection $Transport $Image.Length $Progress
    for($offset=0;$offset -lt $Image.Length;$offset+=128){
        if(!$offset){& $Progress 'page-address' 0 $Image.Length | Out-Null}
        Set-Avr109Address $Transport $offset
        if(!$offset){& $Progress 'page-header' 0 $Image.Length | Out-Null}
        [void]$Transport.Send([byte[]]@(66,0,128,70))
        for($part=0;$part -lt 128;$part+=16){
            if(!$offset){& $Progress "payload-$part" 0 $Image.Length | Out-Null}
            [void]$Transport.Send([byte[]]$Image[($offset+$part)..($offset+$part+15)])
            [void]$Transport.Pause()
        }
        Assert-Avr109Ack $Transport "write page $offset"
        & $Progress 'write' ($offset+128) $Image.Length | Out-Null
    }
    for($offset=0;$offset -lt $Image.Length;$offset+=128){
        Set-Avr109Address $Transport $offset
        [void]$Transport.Send([byte[]]@(103,0,128,70))
        $actual=[byte[]]$Transport.Receive(128)
        if($actual.Length -ne 128){throw 'Incomplete flash readback'}
        for($i=0;$i -lt 128;$i++){
            if($actual[$i] -ne $Image[$offset+$i]){throw ('Flash verification mismatch at 0x{0:X4}' -f ($offset+$i))}
        }
        & $Progress 'verify' ($offset+128) $Image.Length | Out-Null
    }
    [void]$Transport.Send([byte[]]@(76)); Assert-Avr109Ack $Transport 'leave programming'
    [void]$Transport.Send([byte[]]@(69)); Assert-Avr109Ack $Transport 'exit'
    return @{written=$Image.Length;verified=$Image.Length;page=128;payload_chunk=16;signature='1E9587'}
}
