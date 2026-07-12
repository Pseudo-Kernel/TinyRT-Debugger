using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace VSGDBVsix
{
    internal static class NativeMethods
    {
        private static readonly Guid CLSID_VSGDBDebugEngine =
            new Guid("c8749761-deb7-44b1-55aa-5f5c9bdb7a11");

        private static readonly Guid IID_IUnknown =
            new Guid("00000000-0000-0000-C000-000000000046");

        private const uint CLSCTX_INPROC_SERVER = 0x1;

        public static async Task CreateVsgdbDebugEngineAsync(
                    AsyncPackage package)
        {
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            object service =
                await package.GetServiceAsync(typeof(SLocalRegistry));

            ILocalRegistry localRegistry =
                service as ILocalRegistry;

            if (localRegistry == null)
            {
                throw new InvalidOperationException(
                    "SLocalRegistry / ILocalRegistry service is unavailable.");
            }

            Guid clsid = CLSID_VSGDBDebugEngine;
            Guid iid = IID_IUnknown;

            IntPtr unknown;

            int hr = localRegistry.CreateInstance(
                clsid,
                null,
                ref iid,
                CLSCTX_INPROC_SERVER,
                out unknown);

            if (ErrorHandler.Failed(hr))
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