#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <Windows.h>

#include <VSGDBCore/GdbRemoteTarget.h>
#include <VSGDBCore/Components.h>

#include "DisassemblyPrinter.h"
#include "CommandProcessor.h"

#pragma comment(lib, "diaguids.lib")



static std::atomic<CommandProcessor*> g_CommandProcessor = nullptr;

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
main(
    int ArgumentCount,
    char** Arguments)
{
    if (!SetConsoleCtrlHandler(ConsoleControlHandler, TRUE))
    {
        std::printf("Warning: failed to install console Ctrl+C handler.\n");
    }

    VSGDBCore::U16 Port = 1234;

    if (ArgumentCount >= 2)
    {
        Port = static_cast<VSGDBCore::U16>(
            std::strtoul(Arguments[1], nullptr, 0));
    }

    std::printf("VSGDB CLI\n");
    std::printf("Connecting to GDB remote target at 127.0.0.1:%u...\n", Port);

    VSGDBCore::GdbRemoteTarget Target;

    VSGDBCore::DebugTargetConfig Config{};
    Config.Host = L"127.0.0.1";
    Config.Port = Port;

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

    //auto Event = Target.WaitForEvent(0);
    //if (Event.HasValue())
    //{
    //    std::wprintf(
    //        L"Stop: %s\n",
    //        Event.Value.Description.c_str());
    //}



    CommandProcessor Processor(
        Target,
        VSGDBCore::CreateDefaultDisassembler(),
        VSGDBCore::CreateModuleManager(),
        VSGDBCore::CreateDefaultSymbolManager());

    g_CommandProcessor.store(&Processor, std::memory_order_release);

    const int ExitCode = Processor.Run();

    SetConsoleCtrlHandler(ConsoleControlHandler, FALSE);
    g_CommandProcessor.store(nullptr, std::memory_order_release);

    return ExitCode;
}