using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

// Read-only, device-filtered Raw Input. No hooks, injection, cursor warping,
// exclusive opens, or legacy-input suppression. A message-only window is used.
public sealed class MouseboxRawInputMonitor : NativeWindow, IDisposable
{
    const uint WM_INPUT = 0xFF, RID_INPUT = 0x10000003, RIDI_DEVICENAME = 0x20000007;
    const uint RIDEV_INPUTSINK = 0x100, RIDEV_REMOVE = 1;
    [StructLayout(LayoutKind.Sequential)] struct RawDevice { public ushort page, usage; public uint flags; public IntPtr target; }
    [StructLayout(LayoutKind.Sequential)] struct Message {
        public IntPtr hwnd; public uint message; public UIntPtr wParam; public IntPtr lParam;
        public uint time; public int x, y; public uint extra;
    }
    [DllImport("user32.dll", SetLastError=true)] static extern bool RegisterRawInputDevices(RawDevice[] devices, uint count, uint size);
    [DllImport("user32.dll", SetLastError=true)] static extern uint GetRawInputData(IntPtr input, uint command, IntPtr data, ref uint size, uint header);
    [DllImport("user32.dll", SetLastError=true)] static extern uint GetRawInputBuffer(IntPtr data, ref uint size, uint header);
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern uint GetRawInputDeviceInfo(IntPtr device, uint command, StringBuilder data, ref uint size);
    [DllImport("user32.dll")] static extern bool PeekMessage(out Message message, IntPtr window, uint min, uint max, uint remove);
    [DllImport("user32.dll")] static extern IntPtr DispatchMessage(ref Message message);
    [DllImport("user32.dll")] static extern uint MsgWaitForMultipleObjectsEx(uint count, IntPtr handles, uint milliseconds, uint mask, uint flags);
    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern bool SetWindowText(IntPtr window, string text);

    readonly IntPtr buffer = Marshal.AllocHGlobal(16384);
    readonly Dictionary<IntPtr,string> names = new Dictionary<IntPtr,string>();
    readonly string match;
    readonly Stopwatch clock = Stopwatch.StartNew();
    long count, moved, buttons, batches, maximumBatch, lastCount;
    long foregroundCount, lastForegroundCount;
    readonly bool visible;
    double lastReportMs;
    bool disposed, closed;
    TextWriter log;
    // Optional diagnostics observers. Defaults retain the original monitor;
    // no additional clock reads, cursor calls or device queries without them.
    public Action<long,int,int,string> ReportObserved;
    public Action<long> PeriodicObservation;
    void Emit(string line) { Console.WriteLine(line); if (log!=null) { log.WriteLine(line); log.Flush(); } }

    public MouseboxRawInputMonitor(string deviceMatch, bool foreground) {
        if (IntPtr.Size != 8) throw new NotSupportedException("Run the monitor in 64-bit PowerShell.");
        match = deviceMatch;
        visible = foreground;
        CreateHandle(foreground ? new CreateParams {
            Caption="鼠标盒子回报率测试：点击此窗口，保持前台并持续快速移动鼠标",
            X=120, Y=120, Width=780, Height=320, Style=0x10CF0000
        } : new CreateParams { Caption="Mousebox read-only Raw Input", Parent=new IntPtr(-3) });
        var devices = new[] { new RawDevice { page=1, usage=2, flags=RIDEV_INPUTSINK, target=Handle } };
        if (!RegisterRawInputDevices(devices,1,(uint)Marshal.SizeOf(typeof(RawDevice))))
            throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    }
    string DeviceName(IntPtr device) {
        string name;
        if (names.TryGetValue(device,out name)) return name;
        uint size=0;
        GetRawInputDeviceInfo(device,RIDI_DEVICENAME,null,ref size);
        var text = new StringBuilder((int)size + 1);
        if (size==0 || GetRawInputDeviceInfo(device,RIDI_DEVICENAME,text,ref size)==uint.MaxValue) name="";
        else name=text.ToString();
        names.Add(device,name);
        if (name.IndexOf(match,StringComparison.OrdinalIgnoreCase)>=0) Emit("RAW_DEVICE " + name);
        return name;
    }
    bool Record(IntPtr item) {
        if (Marshal.ReadInt32(item,0)!=0 || Marshal.ReadInt32(item,4)<48) return false;
        IntPtr device=Marshal.ReadIntPtr(item,8);
        if (device==IntPtr.Zero || DeviceName(device).IndexOf(match,StringComparison.OrdinalIgnoreCase)<0) return false;
        ++count;
        if (GetForegroundWindow()==Handle) ++foregroundCount;
        if (Marshal.ReadInt32(item,36)!=0 || Marshal.ReadInt32(item,40)!=0) ++moved;
        if (Marshal.ReadInt16(item,28)!=0) ++buttons;
        if (ReportObserved!=null)
            ReportObserved(Stopwatch.GetTimestamp(),Marshal.ReadInt32(item,36),Marshal.ReadInt32(item,40),DeviceName(device));
        return true;
    }
    // The WinForms Message type is explicitly qualified to avoid the MSG alias.
    protected override void WndProc(ref System.Windows.Forms.Message m) {
        if ((uint)m.Msg==WM_INPUT) {
            uint size=16384;
            long batch=0;
            uint bytes=GetRawInputData(m.LParam,RID_INPUT,buffer,ref size,24);
            if (bytes!=uint.MaxValue && bytes>=48 && Record(buffer)) ++batch;
            while (true) {
                size=16384;
                uint items=GetRawInputBuffer(buffer,ref size,24);
                if (items==uint.MaxValue) throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
                if (items==0) break;
                IntPtr cursor=buffer;
                for (uint i=0;i<items;++i) {
                    int block=Marshal.ReadInt32(cursor,4);
                    if (block<24 || block>16384) throw new InvalidOperationException("Invalid RAWINPUT block.");
                    if (Record(cursor)) ++batch;
                    cursor=IntPtr.Add(cursor,(block+7)&~7);
                }
            }
            if (batch>0) { ++batches; if (batch>maximumBatch) maximumBatch=batch; }
        }
        if ((uint)m.Msg==0x10) { closed=true; DestroyHandle(); return; }
        base.WndProc(ref m);
    }
    void Report(bool final) {
        double now=clock.Elapsed.TotalMilliseconds;
        double duration=now-lastReportMs;
        Emit(string.Format(System.Globalization.CultureInfo.InvariantCulture,
            "RAW_INPUT t_ms={0:F1} window_ms={1:F1} reports={2} window_reports={3} hz={4:F1} moved={5} buttons={6} batches={7} max_batch={8} foreground={9} window_foreground_reports={10} final={11}",
            now,duration,count,count-lastCount,duration>0?(count-lastCount)*1000.0/duration:0,moved,buttons,batches,maximumBatch,
            GetForegroundWindow()==Handle?1:0,foregroundCount-lastForegroundCount,final?1:0));
        if (visible && !closed) SetWindowText(Handle,string.Format(System.Globalization.CultureInfo.InvariantCulture,
            "鼠标盒子回报率测试 | {0:F1} Hz | 前台 {1} | 请保持此窗口前台并持续移动鼠标",
            duration>0?(count-lastCount)*1000.0/duration:0,GetForegroundWindow()==Handle?"是":"否"));
        lastCount=count; lastReportMs=now; lastForegroundCount=foregroundCount;
    }
    public void Run(int seconds, string logPath) {
        if (!string.IsNullOrEmpty(logPath)) log = new StreamWriter(new FileStream(logPath,FileMode.CreateNew,FileAccess.Write,FileShare.Read));
        clock.Restart();
        Emit(string.Format("RAW_START filter={0} seconds={1} mode={2}; Hz counts actual Raw Input records, not callbacks or browser events. Background delivery may be coalesced by Windows.",match,seconds,visible?"foreground-window":"background"));
        while (!closed && clock.Elapsed.TotalSeconds<seconds) {
            Message message;
            while (PeekMessage(out message,IntPtr.Zero,0,0,1)) DispatchMessage(ref message);
            if (PeriodicObservation!=null) PeriodicObservation(Stopwatch.GetTimestamp());
            if (clock.Elapsed.TotalMilliseconds-lastReportMs>=1000) Report(false);
            MsgWaitForMultipleObjectsEx(0,IntPtr.Zero,25,0x4FF,4);
        }
        Report(true);
    }
    public void Dispose() {
        if (disposed) return;
        var devices=new[] { new RawDevice { page=1,usage=2,flags=RIDEV_REMOVE,target=IntPtr.Zero } };
        RegisterRawInputDevices(devices,1,(uint)Marshal.SizeOf(typeof(RawDevice)));
        if (!closed) DestroyHandle(); Marshal.FreeHGlobal(buffer); if (log!=null) log.Dispose(); disposed=true;
    }
}
