using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.ComponentModel.Design;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using Task = System.Threading.Tasks.Task;

namespace VSGDBVsix
{
    internal sealed class StartDebugSessionCommand
    {
        public const int CommandId = 0x0101;

        public static readonly Guid CommandSet =
            new Guid("59f79947-34d9-41d9-90a3-2f2342317ee8");

        private static readonly Guid GUID_VSGDBDebugEngine =
            new Guid("9d760574-6f75-4c72-55aa-4b721e3c8c02");

        private readonly AsyncPackage Package_;

        private StartDebugSessionCommand(
            AsyncPackage package,
            OleMenuCommandService commandService)
        {
            Package_ = package;

            CommandID menuCommandID =
                new CommandID(
                    CommandSet,
                    CommandId);

            MenuCommand menuItem =
                new MenuCommand(
                    Execute,
                    menuCommandID);

            commandService.AddCommand(menuItem);

            System.Diagnostics.Debug.WriteLine(
                "[VSGDBVsix] StartDebugSessionCommand registered");
        }

        public static async Task InitializeAsync(
            AsyncPackage package)
        {
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync(
                package.DisposalToken);

            OleMenuCommandService commandService =
                await package.GetServiceAsync(typeof(IMenuCommandService))
                    as OleMenuCommandService;

            if (commandService == null)
            {
                System.Diagnostics.Debug.WriteLine(
                    "[VSGDBVsix] OleMenuCommandService unavailable for StartDebugSessionCommand");

                return;
            }

            _ = new StartDebugSessionCommand(
                package,
                commandService);
        }

        private void Execute(
            object sender,
            EventArgs e)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            ThreadHelper.JoinableTaskFactory.RunAsync(async delegate
            {
                try
                {
                    await LaunchVsgdbDebugTargetAsync();

                    await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();
                }
                catch (Exception ex)
                {
                    await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

                    VsShellUtilities.ShowMessageBox(
                        Package_,
                        ex.ToString(),
                        "VSGDB session start failed",
                        OLEMSGICON.OLEMSGICON_CRITICAL,
                        OLEMSGBUTTON.OLEMSGBUTTON_OK,
                        OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
                }
            }).FileAndForget("VSGDB/StartDebugSession");
        }

        private static IntPtr AllocGuidArray(Guid[] guids)
        {
            int guidSize =
                Marshal.SizeOf(typeof(Guid));

            IntPtr buffer =
                Marshal.AllocCoTaskMem(
                    guidSize * guids.Length);

            for (int i = 0; i < guids.Length; ++i)
            {
                IntPtr element =
                    IntPtr.Add(
                        buffer,
                        i * guidSize);

                Marshal.StructureToPtr(
                    guids[i],
                    element,
                    false);
            }

            return buffer;
        }

        private async Task LaunchVsgdbDebugTargetAsync()
        {
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            object service =
                await Package_.GetServiceAsync(typeof(SVsShellDebugger));

            IVsDebugger4 debugger4 =
                service as IVsDebugger4;

            if (debugger4 == null)
            {
                throw new InvalidOperationException(
                    "SVsShellDebugger / IVsDebugger4 service is unavailable.");
            }

            IntPtr debugEngines = IntPtr.Zero;

            try
            {
                debugEngines =
                    AllocGuidArray(
                        new[] { GUID_VSGDBDebugEngine });

                VsDebugTargetInfo4[] targets =
                    new VsDebugTargetInfo4[1];

                targets[0] = new VsDebugTargetInfo4();

                targets[0].dlo =
                    (uint)DEBUG_LAUNCH_OPERATION.DLO_CreateProcess;

                string assemblyPath = 
                    typeof(StartDebugSessionCommand).Assembly.Location;

                string packageFolder =
                    Path.GetDirectoryName(assemblyPath);

                string stubPath =
                    Path.Combine(
                        packageFolder,
                        "VSGDBDebugTargetStub.exe");

                targets[0].bstrExe = stubPath;
                targets[0].bstrArg = null;
                targets[0].bstrCurDir =
                    Path.GetDirectoryName(stubPath);

                targets[0].dwProcessId = 0;

                targets[0].guidLaunchDebugEngine = GUID_VSGDBDebugEngine;
                targets[0].dwDebugEngineCount = 1;
                targets[0].pDebugEngines = debugEngines;

                targets[0].pUnknown = null; // ProgramNodeObject_;

                targets[0].guidPortSupplier = Guid.Empty;
                targets[0].bstrPortName = null;
                targets[0].bstrOptions = null;
                targets[0].fSendToOutputWindow = false;

                targets[0].guidProcessLanguage = Guid.Empty;
                targets[0].project = null;

                VsDebugTargetProcessInfo[] results =
                    new VsDebugTargetProcessInfo[1];

                System.Diagnostics.Debug.WriteLine(
                    "[VSGDBVsix] Calling IVsDebugger4.LaunchDebugTargets4");

                debugger4.LaunchDebugTargets4(
                    1,
                    targets,
                    results);

                System.Diagnostics.Debug.WriteLine(
                    "[VSGDBVsix] IVsDebugger4.LaunchDebugTargets4 returned");

                DumpLaunchResult(results[0]);
            }
            finally
            {
                if (debugEngines != IntPtr.Zero)
                {
                    Marshal.FreeCoTaskMem(debugEngines);
                }

                // FIXME: End ProgramNode lifetime when LaunchDebugTargets4() fails
            }
        }

        private static void DumpLaunchResult(VsDebugTargetProcessInfo result)
        {
            foreach (var field in typeof(VsDebugTargetProcessInfo).GetFields())
            {
                object value = field.GetValue(result);

                System.Diagnostics.Debug.WriteLine(
                    "[VSGDBVsix] LaunchResult." +
                    field.Name +
                    " = " +
                    (value ?? "(null)"));
            }
        }
    }
}