using System;
using System.Runtime.InteropServices;

namespace VSGDBVsix
{
    internal static class NativeMethods
    {
        private static readonly Guid CLSID_VSGDBDebugEngine =
            new Guid("c8749761-deb7-44b1-55aa-5f5c9bdb7a11");

        private static readonly Guid GUID_VSGDBDebugEngine =
            new Guid("9d760574-6f75-4c72-55aa-4b721e3c8c02");

        private static readonly Guid IID_IUnknown =
            new Guid("00000000-0000-0000-C000-000000000046");

        [DllImport("ole32.dll", ExactSpelling = true)]
        private static extern int CoCreateInstance(
            [In] ref Guid rclsid,
            IntPtr pUnkOuter,
            uint dwClsContext,
            [In] ref Guid riid,
            out IntPtr ppv);

        private const uint CLSCTX_INPROC_SERVER = 0x1;

        public static void CoCreateVsgdbDebugEngine()
        {
            Guid clsid = CLSID_VSGDBDebugEngine;
            Guid iid = IID_IUnknown;

            int hr = CoCreateInstance(
                ref clsid,
                IntPtr.Zero,
                CLSCTX_INPROC_SERVER,
                ref iid,
                out IntPtr unknown);

            if (hr < 0)
            {
                Marshal.ThrowExceptionForHR(hr);
            }

            if (unknown != IntPtr.Zero)
            {
                Marshal.Release(unknown);
            }
        }
    }
}