using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;

// Read-only Windows observation. Never opens CDC/HID, changes device policy,
// resets a device, constrains the cursor, installs a hook, or injects input.
public static class MouseboxWakeupObservation
{
    [StructLayout(LayoutKind.Sequential)] struct PropertyKey { public Guid format; public uint id; }
    [StructLayout(LayoutKind.Sequential)] struct Point { public int x, y; }
    [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
    static extern uint CM_Locate_DevNodeW(out uint node, string instance, uint flags);
    [DllImport("cfgmgr32.dll", CharSet=CharSet.Unicode)]
    static extern uint CM_Get_DevNode_PropertyW(uint node, ref PropertyKey key, out uint type, byte[] data, ref uint size, uint flags);
    [DllImport("user32.dll")] static extern bool GetCursorPos(out Point point);

    sealed class PowerDevice
    {
        public readonly string instance;
        uint node;
        readonly byte[] buffer = new byte[64];
        public PowerDevice(string value) { instance=value; }
        public string Read()
        {
            uint status=CM_Locate_DevNodeW(out node,instance,0);
            if (status!=0) return "locate:" + status.ToString("X");
            var key=new PropertyKey { format=new Guid("a45c254e-df1c-4efd-8020-67d146a850e0"), id=32 };
            uint size=(uint)buffer.Length, type;
            status=CM_Get_DevNode_PropertyW(node,ref key,out type,buffer,ref size,0);
            if (status!=0) return "property:" + status.ToString("X");
            if (type!=0x1003 || size<8 || BitConverter.ToUInt32(buffer,0)<8 || BitConverter.ToUInt32(buffer,0)>size) return "invalid";
            uint state=BitConverter.ToUInt32(buffer,4);
            return state>=1 && state<=4 ? "D" + (state-1).ToString(CultureInfo.InvariantCulture) : "unknown:" + state;
        }
    }

    public static void Validate(string[] instances)
    {
        if (instances.Length==0) throw new ArgumentException("No device instances selected");
        foreach (string instance in instances)
            Console.WriteLine("POWER_DEVICE " + instance + " state=" + new PowerDevice(instance).Read());
        Point point;
        if (!GetCursorPos(out point)) throw new InvalidOperationException("GetCursorPos unavailable");
        Console.WriteLine("READY: read-only power properties and cursor access; no device handle or input window opened.");
    }

    static StreamWriter NewWriter(string path)
    {
        return new StreamWriter(new FileStream(path,FileMode.CreateNew,FileAccess.Write,FileShare.Read,65536));
    }

    public static void Run(int seconds,string match,string[] instances,string prefix)
    {
        if (seconds<1 || seconds>60) throw new ArgumentOutOfRangeException("seconds");
        var powers=new List<PowerDevice>();
        foreach (string instance in instances) powers.Add(new PowerDevice(instance));
        long records=0, dropped=0, powerSamples=0, lastPower=0;
        Exception observationFailure=null;
        const long MaximumRawRows=120000;
        using (var raw=NewWriter(prefix+"-raw.csv"))
        using (var power=NewWriter(prefix+"-power.csv"))
        using (var metadata=NewWriter(prefix+"-metadata.log"))
        using (var monitor=new MouseboxRawInputMonitor(match,false))
        {
            long origin=Stopwatch.GetTimestamp();
            metadata.WriteLine("start=" + DateTimeOffset.UtcNow.ToOffset(TimeSpan.FromHours(8)).ToString("o"));
            metadata.WriteLine("qpc_origin="+origin+" qpc_frequency="+Stopwatch.Frequency);
            metadata.WriteLine("CDC/HID handles are not opened; power is Windows-reported most-recent state, not USB-wire state.");
            metadata.WriteLine("Raw timestamps are local delivery observations; background buffering/coalescing can occur. No physical-motion start timestamp exists.");
            raw.WriteLine("qpc,device,dx,dy");
            power.Write("qpc,cursor_x,cursor_y");
            for (int i=0;i<powers.Count;++i) {
                power.Write(",power_"+i);
                metadata.WriteLine("power_"+i+"="+powers[i].instance);
            }
            power.WriteLine(); metadata.Flush();
            monitor.ReportObserved=(qpc,dx,dy,device)=> {
                if (observationFailure!=null) return;
                if (records>=MaximumRawRows) { ++dropped; return; }
                try {
                    ++records;
                    raw.Write(qpc); raw.Write(','); raw.Write('"'); raw.Write(device.Replace("\"","\"\""));
                    raw.Write("\","); raw.Write(dx); raw.Write(','); raw.WriteLine(dy);
                } catch (Exception error) {
                    // Never escape a native window callback into a WinForms
                    // error dialog. Report failure outside message dispatch.
                    observationFailure=error;
                }
            };
            monitor.PeriodicObservation=qpc=> {
                if (lastPower!=0 && qpc-lastPower<Stopwatch.Frequency/10) return;
                lastPower=qpc; ++powerSamples;
                Point point;
                bool located=GetCursorPos(out point);
                power.Write(qpc); power.Write(',');
                power.Write(located ? point.x.ToString(CultureInfo.InvariantCulture) : "unknown"); power.Write(',');
                power.Write(located ? point.y.ToString(CultureInfo.InvariantCulture) : "unknown");
                foreach (PowerDevice device in powers) { power.Write(','); power.Write(device.Read()); }
                power.WriteLine();
                // Bound disk buffering while keeping it off the per-report path.
                if (powerSamples%10==0) { power.Flush(); raw.Flush(); }
            };
            monitor.Run(seconds,prefix+"-summary.log");
            if (observationFailure!=null) throw new IOException("Raw observation could not be saved.",observationFailure);
            metadata.WriteLine("end_qpc="+Stopwatch.GetTimestamp()+" raw_rows="+records+" raw_rows_dropped="+dropped+" power_samples="+powerSamples);
            Console.WriteLine("WAKEUP_CAPTURE raw_rows="+records+" dropped="+dropped+" power_samples="+powerSamples);
        }
    }
}
