param(
    [Parameter(Mandatory = $true)][ValidatePattern('^COM[1-9][0-9]*$')][string]$Port,
    [Parameter(Mandatory = $true)][ValidateSet(115200, 500000)][int]$BaudRate,
    [ValidateRange(3, 30)][int]$Seconds = 12,
    [Parameter(Mandatory = $true)][string]$OutputPath
)
$ErrorActionPreference = 'Stop'
$taskDevice = @(Get-CimInstance Win32_SerialPort | Where-Object {
    $_.DeviceID -eq $Port -and $_.PNPDeviceID -match '^USB\\VID_10C4&PID_EA60\\'
})
if ($taskDevice.Count -ne 1) { throw 'Expected independent CP2102 TTL device is missing; refusing native USB or a different device' }
if (Test-Path -LiteralPath $OutputPath) { throw 'Observation output already exists' }
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
public sealed class BoxFeedbackSample {
    public long AtMs;
    public uint Sequence;
    public ushort Buttons, Releases;
    public byte Flags;
    public bool Ready { get { return (Flags & 1) != 0; } }
    public bool Known { get { return (Flags & 2) != 0; } }
}
public sealed class BoxFeedbackObserver {
    readonly List<byte> pending = new List<byte>();
    public readonly List<BoxFeedbackSample> Samples = new List<BoxFeedbackSample>();
    public int BadFrames, Discarded;
    static ushort Crc(byte[] data, int count) {
        ushort crc=0xFFFF;
        for (int i=0;i<count;++i) {
            crc ^= (ushort)(data[i]<<8);
            for(int bit=0;bit<8;++bit) crc=(ushort)((crc&0x8000)!=0?(crc<<1)^0x1021:crc<<1);
        }
        return crc;
    }
    static ushort U16(byte[] b,int at) { return (ushort)(b[at]|b[at+1]<<8); }
    public void Feed(byte[] data,int count,long at) {
        for(int i=0;i<count;++i) pending.Add(data[i]);
        while(pending.Count>=14) {
            if(pending[0]!=0xB6 || pending[1]!=0x4D) { pending.RemoveAt(0); ++Discarded; continue; }
            var b=pending.GetRange(0,14).ToArray();
            var sequence=(uint)(b[4]|b[5]<<8|b[6]<<16|b[7]<<24);
            if(b[2]!=1 || (b[3]&~15)!=0 || ((b[3]&8)!=0 && (b[3]&4)==0) || sequence==0 || U16(b,12)!=Crc(b,12)) {
                pending.RemoveAt(0); ++BadFrames; continue;
            }
            Samples.Add(new BoxFeedbackSample {AtMs=at,Sequence=sequence,Buttons=U16(b,8),Releases=U16(b,10),Flags=b[3]});
            pending.RemoveRange(0,14);
        }
    }
    // Deliberately exposes only STOP and feedback-enable PROBE, with zero motion fields.
    public static byte[] Stop() { return Command(3,0,0); }
    public static byte[] EnableFeedback() { return Command(4,0x53494445,1); }
    static byte[] Command(byte type,uint session,uint sequence) {
        var b=new byte[24]; b[0]=0xA5;b[1]=0x5A;b[2]=1;b[3]=type;
        Array.Copy(BitConverter.GetBytes(session),0,b,4,4);
        Array.Copy(BitConverter.GetBytes(sequence),0,b,8,4);
        Array.Copy(BitConverter.GetBytes(Crc(b,22)),0,b,22,2);
        return b;
    }
}
'@
$taskObserver = [BoxFeedbackObserver]::new()
$taskSerial = [IO.Ports.SerialPort]::new($Port, $BaudRate)
$taskSerial.ReadTimeout = 100
$taskSerial.WriteTimeout = 200
$taskSerial.DtrEnable = $false
$taskSerial.RtsEnable = $false
$taskError = $null
$taskWatch = [Diagnostics.Stopwatch]::new()
try {
    $taskSerial.Open()
    foreach ($taskFrame in @([BoxFeedbackObserver]::Stop(), [BoxFeedbackObserver]::EnableFeedback())) {
        $taskSerial.Write([byte[]]$taskFrame, 0, $taskFrame.Length)
    }
    $taskWatch.Start()
    $taskBuffer = [byte[]]::new(4096)
    while ($taskWatch.Elapsed.TotalSeconds -lt $Seconds) {
        $taskAvailable = [Math]::Min($taskSerial.BytesToRead, $taskBuffer.Length)
        if ($taskAvailable -gt 0) {
            $taskRead = $taskSerial.Read($taskBuffer, 0, $taskAvailable)
            $taskObserver.Feed($taskBuffer, $taskRead, $taskWatch.ElapsedMilliseconds)
        }
        Start-Sleep -Milliseconds 2
    }
} catch {
    $taskError = $_.Exception.Message
} finally {
    $taskSerial.Dispose()
    [ordered]@{
        recorded_at = (Get-Date).ToString('o')
        port = $Port
        pnp_identity = $taskDevice[0].PNPDeviceID
        baud_rate = $BaudRate
        duration_ms = $taskWatch.ElapsedMilliseconds
        commands_sent = @('STOP', 'feedback-enable PROBE')
        motion_commands_sent = 0
        bad_frames = $taskObserver.BadFrames
        discarded_bytes = $taskObserver.Discarded
        error = $taskError
        samples = @($taskObserver.Samples.ToArray())
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputPath -Encoding utf8
}
if ($taskError) { throw $taskError }
Write-Output ('Observed ' + $taskObserver.Samples.Count + ' CRC-valid feedback frames; no movement commands')
