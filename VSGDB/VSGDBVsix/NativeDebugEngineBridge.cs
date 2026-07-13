using System;
using System.ComponentModel;
using System.IO;
using System.Runtime.InteropServices;

namespace VSGDBVsix
{
    internal static class NativeDebugEngineBridge
    {
        private delegate int VsgdbCreateDebugProgramNodeDelegate(
            out IntPtr programNode);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr LoadLibraryW(
            string fileName);

        [DllImport("kernel32.dll", CharSet = CharSet.Ansi, SetLastError = true)]
        private static extern IntPtr GetProcAddress(
            IntPtr module,
            string procName);

        public static IntPtr LoadDebugEngineDll()
        {
            string assemblyPath =
                typeof(NativeDebugEngineBridge).Assembly.Location;

            string packageFolder =
                Path.GetDirectoryName(assemblyPath);

            string dllPath =
                Path.Combine(
                    packageFolder,
                    "VSGDBDebugEngine.dll");

            IntPtr module =
                LoadLibraryW(dllPath);

            if (module == IntPtr.Zero)
            {
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "LoadLibraryW failed for " + dllPath);
            }

            return module;
        }

        public static IntPtr CreateDebugProgramNode()
        {
            IntPtr module = LoadDebugEngineDll();

            IntPtr proc =
                GetProcAddress(
                    module,
                    "VsgdbCreateDebugProgramNode");

            if (proc == IntPtr.Zero)
            {
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "GetProcAddress(VsgdbCreateDebugProgramNode) failed.");
            }

            var create =
                (VsgdbCreateDebugProgramNodeDelegate)
                    Marshal.GetDelegateForFunctionPointer(
                        proc,
                        typeof(VsgdbCreateDebugProgramNodeDelegate));

            int hr =
                create(
                    out IntPtr programNode);

            if (hr < 0)
            {
                Marshal.ThrowExceptionForHR(hr);
            }

            if (programNode == IntPtr.Zero)
            {
                throw new InvalidOperationException(
                    "VsgdbCreateDebugProgramNode returned a null program node.");
            }

            return programNode;
        }
    }
}