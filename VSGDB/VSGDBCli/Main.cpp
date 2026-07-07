#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <filesystem>

#include <Windows.h>

#include <VSGDBCore/GdbRemoteTarget.h>
#include <VSGDBCore/Components.h>

#include "DisassemblyPrinter.h"
#include "CommandProcessor.h"
#include "CommandScript.h"

#pragma comment(lib, "diaguids.lib")


struct CliOptions
{
    std::wstring Host = L"127.0.0.1";
    VSGDBCore::U16 Port = 1234;
    std::wstring InitScriptPath;
};


static std::atomic<CommandProcessor*> g_CommandProcessor = nullptr;

static std::wstring
FindDefaultInitScript()
{
    const std::filesystem::path Path = L".vsgdbinit";

    if (std::filesystem::exists(Path))
    {
        return Path.wstring();
    }

    return L"";
}

static BOOL WINAPI
ConsoleControlHandler(
    DWORD ControlType)
{
    if (ControlType != CTRL_C_EVENT &&
        ControlType != CTRL_BREAK_EVENT)
    {
        return FALSE;
    }

    CommandProcessor* Processor =
        g_CommandProcessor.load(std::memory_order_acquire);

    if (Processor == nullptr)
    {
        return FALSE;
    }

    if (Processor->RequestBreakFromConsoleControlHandler())
    {
        return TRUE;
    }

    //
    // At prompt, consume Ctrl+C so the debugger does not terminate.
    // Later we can use this to cancel the current input line.
    //
    return TRUE;
}



int
wmain(
    int argc,
    wchar_t** argv)
{
    if (!SetConsoleCtrlHandler(ConsoleControlHandler, TRUE))
    {
        std::printf("Warning: failed to install console Ctrl+C handler.\n");
    }

    CliOptions Options;

    for (int i = 1; i < argc; i++)
    {
        if (!_wcsicmp(argv[i], L"--host") && i + 1 < argc)
        {
            Options.Host = argv[++i];
        }
        else if (!_wcsicmp(argv[i], L"--port") && i + 1 < argc)
        {
            Options.Port = static_cast<VSGDBCore::U16>(
                std::wcstoul(argv[++i], nullptr, 0));
        }
        else if (!_wcsicmp(argv[i], L"--init") && i + 1 < argc)
        {
            Options.InitScriptPath = argv[++i];
        }
    }

    if (Options.InitScriptPath.empty())
    {
        Options.InitScriptPath = FindDefaultInitScript();
    }

    std::printf("VSGDB CLI\n");
    std::wprintf(L"Connecting to GDB remote target at %s:%u...\n", 
        Options.Host.c_str(), Options.Port);

    VSGDBCore::GdbRemoteTarget Target;

    VSGDBCore::DebugTargetConfig Config{};
    Config.Host = Options.Host;
    Config.Port = Options.Port;

    auto Error = Target.Connect(Config);
    if (!Error.IsSuccess())
    {
        std::wprintf(
            L"Connect failed: %s Code=%d Native=0x%08x\n",
            Error.Message.c_str(),
            static_cast<int>(Error.Code),
            Error.NativeCode);

        return 1;
    }

    std::printf("Connected.\n");

    auto Event = Target.WaitForEvent(0);
    if (Event.HasValue())
    {
        std::wprintf(
            L"Stop: %s\n",
            Event.Value.Description.c_str());
    }

    auto Disassembler = VSGDBCore::CreateDefaultDisassembler();
    auto ModuleManager = VSGDBCore::CreateModuleManager();
    auto SymbolManager = VSGDBCore::CreateDefaultSymbolManager();
    auto StackWalker = VSGDBCore::CreateX64PeUnwindStackWalker(ModuleManager.get());

    CommandProcessor Processor(
        Target,
        std::move(Disassembler),
        std::move(ModuleManager),
        std::move(SymbolManager),
        std::move(StackWalker)
//        VSGDBCore::CreateX64FramePointerStackWalker()
    );

    g_CommandProcessor.store(&Processor, std::memory_order_release);

    if (!Options.InitScriptPath.empty())
    {
        if (!ExecuteCommandScript(
            Processor,
            Options.InitScriptPath))
        {
            return 1;
        }
    }

    const int ExitCode = Processor.Run();

    SetConsoleCtrlHandler(ConsoleControlHandler, FALSE);
    g_CommandProcessor.store(nullptr, std::memory_order_release);

    return ExitCode;
}