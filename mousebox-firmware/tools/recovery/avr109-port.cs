using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace MouseboxFirmware
{
    public sealed class Avr109Device
    {
        public string Port { get; set; }
        public string InstanceId { get; set; }
    }

    public static class Avr109Devices
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct DeviceInfo
        {
            public int Size;
            public Guid ClassGuid;
            public uint DevInst;
            public IntPtr Reserved;
        }

        [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr SetupDiGetClassDevsW(ref Guid guid, string enumerator, IntPtr parent, uint flags);
        [DllImport("setupapi.dll", SetLastError = true)]
        private static extern bool SetupDiEnumDeviceInfo(IntPtr set, uint index, ref DeviceInfo info);
        [DllImport("setupapi.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern bool SetupDiGetDeviceInstanceIdW(IntPtr set, ref DeviceInfo info,
            System.Text.StringBuilder id, int capacity, out int required);
        [DllImport("setupapi.dll", SetLastError = true)]
        private static extern IntPtr SetupDiOpenDevRegKey(IntPtr set, ref DeviceInfo info,
            uint scope, uint profile, uint type, uint access);
        [DllImport("setupapi.dll")]
        private static extern bool SetupDiDestroyDeviceInfoList(IntPtr set);
        [DllImport("advapi32.dll", CharSet = CharSet.Unicode)]
        private static extern int RegQueryValueExW(IntPtr key, string name, IntPtr reserved,
            out uint type, byte[] data, ref uint size);
        [DllImport("advapi32.dll")]
        private static extern int RegCloseKey(IntPtr key);

        public static Avr109Device[] Present()
        {
            Guid guid = new Guid("4D36E978-E325-11CE-BFC1-08002BE10318");
            IntPtr set = SetupDiGetClassDevsW(ref guid, null, IntPtr.Zero, 2);
            if (set == new IntPtr(-1)) throw new Win32Exception(Marshal.GetLastWin32Error(), "SetupDiGetClassDevs");
            var devices = new System.Collections.Generic.List<Avr109Device>();
            try
            {
                for (uint index = 0; ; index++)
                {
                    var info = new DeviceInfo { Size = Marshal.SizeOf<DeviceInfo>() };
                    if (!SetupDiEnumDeviceInfo(set, index, ref info))
                    {
                        int error = Marshal.GetLastWin32Error();
                        if (error == 259) break;
                        throw new Win32Exception(error, "SetupDiEnumDeviceInfo");
                    }
                    var id = new System.Text.StringBuilder(1024);
                    int required;
                    if (!SetupDiGetDeviceInstanceIdW(set, ref info, id, id.Capacity, out required))
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "SetupDiGetDeviceInstanceId");
                    IntPtr key = SetupDiOpenDevRegKey(set, ref info, 1, 0, 1, 0x20019);
                    if (key == new IntPtr(-1)) continue;
                    try
                    {
                        var bytes = new byte[256];
                        uint size = (uint)bytes.Length;
                        uint type;
                        if (RegQueryValueExW(key, "PortName", IntPtr.Zero, out type, bytes, ref size) == 0 && type == 1)
                        {
                            string port = System.Text.Encoding.Unicode.GetString(bytes, 0, (int)size).TrimEnd('\0');
                            if (System.Text.RegularExpressions.Regex.IsMatch(port, @"^COM[1-9][0-9]*$"))
                                devices.Add(new Avr109Device { Port = port, InstanceId = id.ToString() });
                        }
                    }
                    finally { RegCloseKey(key); }
                }
            }
            finally { SetupDiDestroyDeviceInfoList(set); }
            return devices.ToArray();
        }
    }

    // Caterina uses USB CDC bytes; no UART baud-rate change is needed here.
    // Keep this recovery transport separate from the production TTL serial port.
    public sealed class Avr109Port : IDisposable
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct Timeouts
        {
            public uint ReadInterval, ReadMultiplier, ReadConstant;
            public uint WriteMultiplier, WriteConstant;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct Operation
        {
            public IntPtr Internal, InternalHigh;
            public uint Offset, OffsetHigh;
            public IntPtr Event;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern SafeFileHandle CreateFileW(string path, uint access, uint share,
            IntPtr security, uint disposition, uint flags, IntPtr template);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetCommTimeouts(SafeFileHandle handle, ref Timeouts timeouts);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ReadFile(SafeFileHandle handle, IntPtr buffer, uint count,
            out uint transferred, IntPtr operation);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WriteFile(SafeFileHandle handle, IntPtr buffer, uint count,
            out uint transferred, IntPtr operation);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetOverlappedResult(SafeFileHandle handle, IntPtr operation,
            out uint transferred, bool wait);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CancelIoEx(SafeFileHandle handle, IntPtr operation);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool EscapeCommFunction(SafeFileHandle handle, uint function);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool DeviceIoControl(SafeFileHandle handle, uint code, ref uint input,
            uint inputSize, IntPtr output, uint outputSize, out uint returned, IntPtr operation);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateEventW(IntPtr attributes, bool manual, bool initial, IntPtr name);
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr handle, uint milliseconds);
        [DllImport("kernel32.dll")]
        private static extern bool CloseHandle(IntPtr handle);

        private readonly SafeFileHandle handle;

        public static void Touch1200(string port)
        {
            if (!System.Text.RegularExpressions.Regex.IsMatch(port, @"^COM[1-9][0-9]*$"))
                throw new ArgumentException("Invalid COM port");
            using (SafeFileHandle reset = CreateFileW(@"\\.\" + port, 0xC0000000, 0, IntPtr.Zero, 3, 0, IntPtr.Zero))
            {
                if (reset.IsInvalid) throw new Win32Exception(Marshal.GetLastWin32Error(), "Reset CreateFile");
                if (!EscapeCommFunction(reset, 5)) throw new Win32Exception(Marshal.GetLastWin32Error(), "Reset SETDTR");
                uint baud = 1200;
                uint returned;
                if (!DeviceIoControl(reset, 0x1B0004, ref baud, 4, IntPtr.Zero, 0, out returned, IntPtr.Zero))
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "Reset SET_BAUD_RATE");
                System.Threading.Thread.Sleep(50);
                if (!EscapeCommFunction(reset, 6)) throw new Win32Exception(Marshal.GetLastWin32Error(), "Reset CLRDTR");
            }
        }

        public Avr109Port(string port)
        {
            if (!System.Text.RegularExpressions.Regex.IsMatch(port, @"^COM[1-9][0-9]*$"))
                throw new ArgumentException("Invalid COM port");
            var readiness = System.Diagnostics.Stopwatch.StartNew();
            while (true)
            {
                handle = CreateFileW(@"\\.\" + port, 0xC0000000, 0, IntPtr.Zero, 3, 0x40000000, IntPtr.Zero);
                if (!handle.IsInvalid) break;
                int error = Marshal.GetLastWin32Error();
                handle.Dispose();
                if (readiness.ElapsedMilliseconds >= 3000 ||
                    (error != 2 && error != 3 && error != 5 && error != 21 && error != 31 && error != 1167))
                    throw new Win32Exception(error, "CreateFile " + port + ": Windows error " + error);
                // Presence can precede the first successful open during driver setup.
                System.Threading.Thread.Sleep(50);
            }
            var timeouts = new Timeouts { ReadInterval = uint.MaxValue, WriteConstant = 1000 };
            if (!SetCommTimeouts(handle, ref timeouts))
            {
                int error = Marshal.GetLastWin32Error();
                handle.Dispose();
                throw new Win32Exception(error, "SetCommTimeouts " + port);
            }
        }

        private int Transfer(byte[] bytes, bool writing)
        {
            if (handle.IsClosed) throw new ObjectDisposedException("AVR109 recovery transport");
            IntPtr signal = CreateEventW(IntPtr.Zero, true, false, IntPtr.Zero);
            if (signal == IntPtr.Zero)
                throw new Win32Exception(Marshal.GetLastWin32Error(), "CreateEvent");
            IntPtr memory = Marshal.AllocHGlobal(bytes.Length);
            IntPtr operation = Marshal.AllocHGlobal(Marshal.SizeOf<Operation>());
            bool releaseResources = true;
            try
            {
                Marshal.StructureToPtr(new Operation { Event = signal }, operation, false);
                if (writing) Marshal.Copy(bytes, 0, memory, bytes.Length);
                uint transferred;
                bool complete = writing
                    ? WriteFile(handle, memory, (uint)bytes.Length, out transferred, operation)
                    : ReadFile(handle, memory, (uint)bytes.Length, out transferred, operation);
                if (!complete)
                {
                    int error = Marshal.GetLastWin32Error();
                    if (error != 997) throw new Win32Exception(error, writing ? "WriteFile" : "ReadFile");
                    if (WaitForSingleObject(signal, 1000) != 0)
                    {
                        CancelIoEx(handle, operation);
                        if (WaitForSingleObject(signal, 250) != 0)
                        {
                            // An unresponsive USB driver may never acknowledge cancellation.
                            // This helper is used only in isolated recovery processes. Abandon
                            // this single handle and its buffers until process teardown instead
                            // of freeing memory still referenced by the driver, blocking in
                            // CloseHandle, or waiting indefinitely in GetOverlappedResult.
                            releaseResources = false;
                            handle.SetHandleAsInvalid();
                            throw new TimeoutException("USB transfer timed out; driver cancellation is still pending. " +
                                "Recovery transport retired; disconnect this board before retrying.");
                        }
                        GetOverlappedResult(handle, operation, out transferred, false);
                        throw new TimeoutException(writing ? "USB write timed out" : "USB read timed out");
                    }
                    if (!GetOverlappedResult(handle, operation, out transferred, false))
                        throw new Win32Exception(Marshal.GetLastWin32Error(), "USB completion");
                }
                if (!writing) Marshal.Copy(memory, bytes, 0, (int)transferred);
                return (int)transferred;
            }
            finally
            {
                if (releaseResources)
                {
                    Marshal.FreeHGlobal(operation);
                    Marshal.FreeHGlobal(memory);
                    CloseHandle(signal);
                }
            }
        }

        public void Send(byte[] bytes)
        {
            if (Transfer(bytes, true) != bytes.Length) throw new InvalidOperationException("Partial USB write");
        }

        public byte[] Receive(int count)
        {
            if (count < 1 || count > 128) throw new ArgumentOutOfRangeException("count");
            var result = new byte[count];
            int used = 0;
            var deadline = System.Diagnostics.Stopwatch.StartNew();
            while (used < count)
            {
                if (deadline.ElapsedMilliseconds > 2000) throw new TimeoutException("USB response timed out");
                var part = new byte[count - used];
                int received = Transfer(part, false);
                Array.Copy(part, 0, result, used, received);
                used += received;
                if (received == 0) System.Threading.Thread.Sleep(1);
            }
            return result;
        }

        public void Dispose() { handle.Dispose(); }
    }
}
