using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using System;
using System.ComponentModel.Design;
using System.Threading;
using Task = System.Threading.Tasks.Task;

namespace VSGDBVsix
{
    internal sealed class StartDebugSessionCommand
    {
        public const int CommandId = 0x0101;

        public static readonly Guid CommandSet =
            new Guid("59f79947-34d9-41d9-90a3-2f2342317ee8");

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

            System.Diagnostics.Debug.WriteLine(
                "[VSGDBVsix] Start VSGDB Debug Session command executed");

            VsShellUtilities.ShowMessageBox(
                Package_,
                "Start VSGDB Debug Session invoked.",
                "VSGDB",
                OLEMSGICON.OLEMSGICON_INFO,
                OLEMSGBUTTON.OLEMSGBUTTON_OK,
                OLEMSGDEFBUTTON.OLEMSGDEFBUTTON_FIRST);
        }
    }
}