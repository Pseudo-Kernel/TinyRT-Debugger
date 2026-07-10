
#include "DisassemblyPrinter.h"
#include "CommandProcessor.h"
#include "ExpressionUtil.h"
#include "RemoteThreadIdUtil.h"
#include "MemoryPrinter.h"
#include "PagingPrinter.h"
#include "PageFaultPrinter.h"
#include "SymbolQuery.h"
#include "CommandScript.h"
#include "PeImageUtil.h"

#include <VSGDBCore/SymbolTypes.h>
#include <VSGDBCore/X64Paging.h>
#include <VSGDBCore/BreakpointTypes.h>
#include <VSGDBCore/Result.h>
#include <cstdio>
#include <cwctype>
#include <sstream>
#include <iostream>
#include <Windows.h>

static constexpr VSGDBCore::U32 MaxCommandScriptDepth = 8;

static constexpr VSGDBCore::U32 DefaultDisassemblyBytes = 0x80;
static constexpr VSGDBCore::U32 MaxDisassemblyBytes = 0x1000;

static constexpr VSGDBCore::U32 DefaultFunctionDisassemblyBytes = 0x100;
static constexpr VSGDBCore::U32 MaxFunctionDisassemblyBytes = 0x1000;

static constexpr size_t MaxSymbolSearchResults = 2048;


class ScopedTargetRunning final
{
public:
    ScopedTargetRunning(
        std::atomic_bool& TargetRunning,
        std::atomic_bool& BreakRequested)
        : TargetRunning(TargetRunning),
        BreakRequested(BreakRequested)
    {
        this->BreakRequested.store(false, std::memory_order_release);
        this->TargetRunning.store(true, std::memory_order_release);
    }

    ~ScopedTargetRunning()
    {
        TargetRunning.store(false, std::memory_order_release);
        BreakRequested.store(false, std::memory_order_release);
    }

private:
    std::atomic_bool& TargetRunning;
    std::atomic_bool& BreakRequested;
};


static std::wstring
ToLower(
    std::wstring Text)
{
    for (wchar_t& Character : Text)
    {
        Character = static_cast<wchar_t>(
            std::towlower(Character));
    }

    return Text;
}

static std::vector<std::wstring>
SplitArguments(
    const std::wstring& Line)
{
    std::wistringstream Stream(Line);

    std::vector<std::wstring> Arguments;
    std::wstring Argument;

    while (Stream >> Argument)
    {
        Arguments.push_back(Argument);
    }

    return Arguments;
}

static bool
ParseU32(
    const std::wstring& Text,
    VSGDBCore::U32& OutValue)
{
    wchar_t* End = nullptr;

    const unsigned long Value = std::wcstoul(
        Text.c_str(),
        &End,
        0);

    if (End == Text.c_str() || *End != L'\0')
    {
        return false;
    }

    OutValue = static_cast<VSGDBCore::U32>(Value);
    return true;
}

static void
PrintStopEvent(
    const VSGDBCore::DebugEvent& Event)
{
    std::wprintf(
        L"Stop: %s\n",
        Event.Description.c_str());

    if (!Event.RemoteThreadId.empty())
    {
        std::printf(
            "Stopped thread: %s\n",
            Event.RemoteThreadId.c_str());
    }
}

static const wchar_t*
BreakpointKindName(
    VSGDBCore::BreakpointKind Kind)
{
    switch (Kind)
    {
    case VSGDBCore::BreakpointKind::Software:
        return L"software";

    case VSGDBCore::BreakpointKind::HardwareExecute:
        return L"hw-exec";

    case VSGDBCore::BreakpointKind::HardwareWrite:
        return L"hw-write";

    case VSGDBCore::BreakpointKind::HardwareAccess:
        return L"hw-access";

    default:
        return L"unknown";
    }
}

static std::wstring
GetFileNameFromPath(
    const std::wstring& Path)
{
    const size_t Pos = Path.find_last_of(L"\\/");

    if (Pos == std::wstring::npos)
    {
        return Path;
    }

    return Path.substr(Pos + 1);
}

static void
PrintDebugError(
    const wchar_t* Prefix,
    const VSGDBCore::DebugError& Error)
{
    if (Error.Message.empty())
    {
        std::wprintf(
            L"%s. Code=%u Native=0x%08x\n",
            Prefix,
            static_cast<unsigned>(Error.Code),
            Error.NativeCode);
    }
    else
    {
        std::wprintf(
            L"%s: %s. Code=%u Native=0x%08x\n",
            Prefix,
            Error.Message.c_str(),
            static_cast<unsigned>(Error.Code),
            Error.NativeCode);
    }
}

static std::wstring
GetModuleDisplayNameById(
    VSGDBCore::IModuleManager& ModuleManager,
    VSGDBCore::ModuleId ModuleId)
{
    auto Module = ModuleManager.GetModuleById(ModuleId);

    if (!Module.HasValue())
    {
        return L"?";
    }

    if (!Module.Value.Name.empty())
    {
        return Module.Value.Name;
    }

    if (!Module.Value.ImagePath.empty())
    {
        return Module.Value.ImagePath;
    }

    return L"?";
}

static bool
StartsWithCaseInsensitive(
    const std::wstring& Text,
    const std::wstring& Prefix)
{
    if (Prefix.size() > Text.size())
    {
        return false;
    }

    for (size_t Index = 0; Index < Prefix.size(); ++Index)
    {
        if (std::towlower(Text[Index]) !=
            std::towlower(Prefix[Index]))
        {
            return false;
        }
    }

    return true;
}

static const wchar_t*
SymbolKindToString(
    VSGDBCore::SymbolKind Kind)
{
    switch (Kind)
    {
    case VSGDBCore::SymbolKind::Function: return L"Func";
    case VSGDBCore::SymbolKind::Data: return L"Data";
    case VSGDBCore::SymbolKind::PublicSymbol: return L"Public";
    case VSGDBCore::SymbolKind::Label: return L"Label";
    default: return L"?";
    }
}


CommandProcessor::CommandProcessor(
    std::unique_ptr<VSGDBCore::IDebugSession> Session,
    std::unique_ptr<VSGDBCore::IDisassembler> Disassembler,
    std::unique_ptr<VSGDBCore::IModuleManager> ModuleManager,
    std::unique_ptr<VSGDBCore::ISymbolManager> SymbolManager,
    std::unique_ptr<VSGDBCore::IStackWalker> StackWalker) :
    Session(std::move(Session)),
    Disassembler(std::move(Disassembler)),
    ModuleManager(std::move(ModuleManager)),
    SymbolManager(std::move(SymbolManager)),
    StackWalker(std::move(StackWalker)),
    Formatter()
{
    Formatter = std::make_unique<DebugTextFormatter>(
        this->ModuleManager.get(),
        this->SymbolManager.get());
}

int
CommandProcessor::Run()
{
    std::wprintf(L"Type 'help' for commands.\n");

    while (!QuitRequested)
    {
        std::wprintf(
            L"vsgdb:cpu%u> ",
            CurrentCpuId);

        std::wstring Line;
        if (!std::getline(std::wcin, Line))
        {
            break;
        }

        ProcessLine(Line);
    }

    ClearAllKnownBreakpoints();

    return 0;
}

bool
CommandProcessor::RequestBreakFromConsoleControlHandler()
{
    //
    // Console control handler runs on a different thread.
    // Only send break if the target is currently running.
    //
    if (!TargetRunning.load(std::memory_order_acquire))
    {
        return false;
    }

    bool Expected = false;
    if (!BreakRequested.compare_exchange_strong(
        Expected,
        true,
        std::memory_order_acq_rel))
    {
        return true;
    }

    //
    // Send raw 0x03 to the GDB remote target.
    // Do not print here. Keep the console control handler path minimal.
    //
    auto Error = Session->BreakExecution();
    (void)Error;

    return true;
}

bool
CommandProcessor::ProcessLine(
    const std::wstring& Line)
{
    const std::vector<std::wstring> Arguments =
        SplitArguments(Line);

    if (Arguments.empty())
    {
        return true;
    }

    return ExecuteCommand(Arguments);
}

bool
CommandProcessor::ExecuteCommand(
    const std::vector<std::wstring>& Arguments)
{
    const std::wstring Command =
        ToLower(Arguments[0]);

    if (Command == L"help" || Command == L"?")
    {
        return ExecuteHelp(Arguments);
    }

    if (Command == L"script")
    {
        return ExecuteScript(Arguments);
    }

    if (Command == L"quit" || Command == L"exit" || Command == L"q")
    {
        return ExecuteQuit(Arguments);
    }

    if (Command == L"threads")
    {
        return ExecuteThreads(Arguments);
    }

    if (Command == L"cpu")
    {
        return ExecuteCpu(Arguments);
    }

    if (Command == L"r")
    {
        return ExecuteRegisters(Arguments);
    }

    if (Command == L"db")
    {
        return ExecuteDumpVirtualBytes(Arguments);
    }

    if (Command == L"pb")
    {
        return ExecuteDumpPhysicalBytes(Arguments);
    }

    if (Command == L"pte")
    {
        return ExecutePte(Arguments);
    }

    if (Command == L"pf")
    {
        return ExecutePageFault(Arguments);
    }

    if (Command == L"step" || Command == L"t")
    {
        return ExecuteStep(Arguments);
    }

    if (Command == L"g" || Command == L"go")
    {
        return ExecuteGo(Arguments);
    }

    if (Command == L"bp")
    {
        return ExecuteBreakpointSet(Arguments);
    }

    if (Command == L"bl")
    {
        return ExecuteBreakpointList(Arguments);
    }

    if (Command == L"bc")
    {
        return ExecuteBreakpointClear(Arguments);
    }

    if (Command == L"bcaddr")
    {
        return ExecuteBreakpointClearAddress(Arguments);
    }

    if (Command == L"u")
    {
        return ExecuteDisassemble(Arguments);
    }

    if (Command == L"uf")
    {
        return ExecuteDisassembleFunction(Arguments);
    }

    if (Command == L"ub")
    {
        return ExecuteDisassembleBackwardWindow(Arguments);
    }

    if (Command == L"lm")
    {
        return ExecuteListModules(Arguments);
    }

    if (Command == L".reload")
    {
        return ExecuteReload(Arguments);
    }

    if (Command == L".symload")
    {
        return ExecuteSymbolLoad(Arguments);
    }

    if (Command == L"x")
    {
        return ExecuteSymbol(Arguments);
    }

    if (Command == L"line")
    {
        return ExecuteSourceLine(Arguments);
    }

    if (Command == L"k")
    {
        return ExecuteStackTrace(Arguments, false);
    }

    if (Command == L"kb")
    {
        return ExecuteStackTrace(Arguments, true);
    }

    std::wprintf(
        L"Unknown command: %s\n",
        Arguments[0].c_str());

    return false;
}

bool
CommandProcessor::ExecuteHelp(
    const std::vector<std::wstring>& Arguments)
{
    (void)Arguments;

    std::wprintf(
        L"Commands:\n"
        L"  help, ?              Show this help.\n"
        L"  script <file>        Execute command script.\n"
        L"  quit, exit, q        Exit debugger.\n"
        L"  threads              List CPUs/vCPUs/threads.\n"
        L"  cpu                  Print current implicit CPU.\n"
        L"  cpu <id>             Set current implicit CPU.\n"
        L"  r [register...]      Print all or selected registers for current CPU.\n"
        L"  db <expr> <size>     Dump virtual bytes from current CPU context.\n"
        L"  pb <expr> <size>     Dump physical bytes.\n"
        L"  pte <expr>           Walk page tables for virtual address.\n"
        L"  pf <va> <error>      Diagnose page fault.\n"
        L"  step                 Single-step current CPU.\n"
        L"  g                    Continue.\n"
        L"  bp <expr>            Set breakpoint.\n"
        L"  bl                   List breakpoints.\n"
        L"  bc <id>              Clear breakpoint.\n"
        L"  bc *                 Clear all breakpoints known to this CLI session.\n"
        L"  bcaddr <expr>        Clear software breakpoint by address. Useful for cleanup after reconnect.\n"
        L"  u [expr] [bytes]     Disassemble instructions starting in byte range.\n"
        L"  uf [expr]            Disassemble containing function.\n"
        L"  ub [expr] [bytes]    Disassemble window around address using function start for alignment.\n"
        L"  lm                   List registered modules.\n"
        L"  .reload <image> <base> [size]   Register a module image at base address.\n"
        L"  .symload <module-id>            Load symbols for registered module.\n"
        L"  x <symbol-name|pattern>         Lookup or search symbols. Supports *, prefix*, *suffix, *contains*.\n"
        L"  line [expr]          Show source location for address.\n"
        L"  k [max]              Show stack backtrace.\n"
        L"  kb [max]             Show verbose stack backtrace.\n"
    );

    return true;
}

bool
CommandProcessor::ExecuteScript(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: script <script-file>\n");
        return false;
    }

    if (ScriptDepth >= MaxCommandScriptDepth)
    {
        std::wprintf(L"Command script nesting is too deep.\n");
        return false;
    }

    ++ScriptDepth;
    const bool Result = ExecuteCommandScript(*this, Arguments[1]);
    --ScriptDepth;

    return Result;
}

bool
CommandProcessor::ExecuteQuit(
    const std::vector<std::wstring>& Arguments)
{
    (void)Arguments;

    ClearAllKnownBreakpoints();

    QuitRequested = true;
    return true;
}

bool
CommandProcessor::ExecuteThreads(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 1)
    {
        std::wprintf(L"Usage: threads\n");
        return false;
    }

    auto Threads = Session->EnumerateThreads();
    if (!Threads.HasValue())
    {
        PrintDebugError(L"EnumerateThreads failed", Threads.Error);
        return false;
    }

    for (const auto& Thread : Threads.Value)
    {
        std::wprintf(
            L"%c CPU%u -> %s\n",
            Thread.CpuId == CurrentCpuId ? L'*' : L' ',
            Thread.CpuId,
            Thread.DisplayName.c_str());
    }

    return true;
}

bool
CommandProcessor::ExecuteCpu(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() == 1)
    {
        std::wprintf(
            L"Current CPU = %u\n",
            CurrentCpuId);

        return true;
    }

    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: cpu [id]\n");
        return false;
    }

    VSGDBCore::U32 CpuId = 0;
    if (!ParseU32(Arguments[1], CpuId))
    {
        std::wprintf(
            L"Invalid CPU ID: %s\n",
            Arguments[1].c_str());

        return false;
    }

    auto Threads = Session->EnumerateThreads();
    if (!Threads.HasValue())
    {
        PrintDebugError(L"EnumerateThreads failed", Threads.Error);
        return false;
    }

    bool Found = false;

    for (const auto& Thread : Threads.Value)
    {
        if (Thread.CpuId == CpuId)
        {
            Found = true;
            break;
        }
    }

    if (!Found)
    {
        std::wprintf(
            L"CPU%u does not exist.\n",
            CpuId);

        return false;
    }

    CurrentCpuId = CpuId;

    std::wprintf(
        L"Current CPU = %u\n",
        CurrentCpuId);

    return true;
}

bool
CommandProcessor::ExecuteRegisters(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.empty())
    {
        std::wprintf(L"Usage: r [register...]\n");
        return false;
    }

    auto Registers = Session->GetRegisters(CurrentCpuId);
    if (!Registers.HasValue())
    {
        PrintDebugError(L"GetRegisters failed", Registers.Error);
        return false;
    }

    if (Arguments.size() == 1)
    {
        Formatter->PrintRegisters(Registers.Value);
        return true;
    }

    bool AllValid = true;

    for (size_t Index = 1; Index < Arguments.size(); ++Index)
    {
        if (!Formatter->PrintRegisterByName(
            Registers.Value,
            Arguments[Index]))
        {
            std::wprintf(
                L"Unknown register: %s\n",
                Arguments[Index].c_str());

            AllValid = false;
        }
    }

    return AllValid;
}

bool
CommandProcessor::ExecuteDumpVirtualBytes(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 3)
    {
        std::wprintf(L"Usage: db <expr> <size>\n");
        return false;
    }

    auto Address = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!Address.HasValue())
    {
        PrintDebugError(L"Failed to evaluate address", Address.Error);
        return false;
    }

    VSGDBCore::U64 Size64 = 0;
    if (!ParseU64(Arguments[2], Size64) || Size64 == 0)
    {
        std::wprintf(
            L"Invalid size: %s\n",
            Arguments[2].c_str());

        return false;
    }

    if (Size64 > 0x10000)
    {
        std::wprintf(
            L"Size too large. Maximum is 0x10000 bytes.\n");

        return false;
    }

    auto Bytes = Session->ReadVirtualMemory(
        CurrentCpuId,
        Address.Value,
        static_cast<VSGDBCore::U32>(Size64));

    if (!Bytes.HasValue())
    {
        PrintDebugError(L"ReadVirtualMemory failed", Bytes.Error);
        return false;
    }

    std::wprintf(
        L"Dumping %u bytes from %s\n",
        static_cast<VSGDBCore::U32>(Size64),
        Formatter->FormatAddressInline(Address.Value).c_str());

    PrintHexDump(
        Address.Value,
        Bytes.Value);

    return true;
}

bool
CommandProcessor::ExecuteDumpPhysicalBytes(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 3)
    {
        std::wprintf(L"Usage: pb <expr> <size>\n");
        return false;
    }

    auto Address = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!Address.HasValue())
    {
        PrintDebugError(L"Failed to evaluate address", Address.Error);
        return false;
    }

    VSGDBCore::U64 Size64 = 0;
    if (!ParseU64(Arguments[2], Size64) || Size64 == 0)
    {
        std::wprintf(
            L"Invalid size: %s\n",
            Arguments[2].c_str());

        return false;
    }

    if (Size64 > 0x10000)
    {
        std::wprintf(
            L"Size too large. Maximum is 0x10000 bytes.\n");

        return false;
    }

    auto Bytes = Session->ReadPhysicalMemory(
        Address.Value,
        static_cast<VSGDBCore::U32>(Size64));

    if (!Bytes.HasValue())
    {
        PrintDebugError(L"ReadPhysicalMemory failed", Bytes.Error);
        return false;
    }

    PrintHexDump(
        Address.Value,
        Bytes.Value);

    return true;
}

bool
CommandProcessor::ExecutePte(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: pte <expr>\n");
        return false;
    }

    auto VirtualAddress = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!VirtualAddress.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate virtual address",
            VirtualAddress.Error);

        return false;
    }

    auto Registers = Session->GetRegisters(CurrentCpuId);
    if (!Registers.HasValue())
    {
        PrintDebugError(
            L"GetRegisters failed",
            Registers.Error);

        return false;
    }

    VSGDBCore::X64PagingContext Paging{};
    Paging.Cr0 = Registers.Value.Cr0;
    Paging.Cr3 = Registers.Value.Cr3;
    Paging.Cr4 = Registers.Value.Cr4;
    Paging.Efer = Registers.Value.Efer;

    auto Translation = VSGDBCore::TranslateX64VirtualAddress(
        *Session,
        Paging,
        VirtualAddress.Value);

    if (!Translation.HasValue())
    {
        PrintDebugError(
            L"TranslateX64VirtualAddress failed",
            Translation.Error);

        return false;
    }

    Formatter->PrintTranslation(Translation.Value);
    return true;
}

bool
CommandProcessor::ExecutePageFault(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 3)
    {
        std::wprintf(L"Usage: pf <va> <error-code>\n");
        return false;
    }

    auto VirtualAddress = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!VirtualAddress.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate virtual address",
            VirtualAddress.Error);

        return false;
    }

    auto ErrorCode = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[2],
        SymbolManager.get());

    if (!ErrorCode.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate page-fault error code:",
            ErrorCode.Error);

        return false;
    }

    auto Registers = Session->GetRegisters(CurrentCpuId);
    if (!Registers.HasValue())
    {
        PrintDebugError(
            L"GetRegisters failed",
            Registers.Error);

        return false;
    }

    VSGDBCore::X64PagingContext Paging{};
    Paging.Cr0 = Registers.Value.Cr0;
    Paging.Cr3 = Registers.Value.Cr3;
    Paging.Cr4 = Registers.Value.Cr4;
    Paging.Efer = Registers.Value.Efer;

    auto Translation = VSGDBCore::TranslateX64VirtualAddress(
        *Session,
        Paging,
        VirtualAddress.Value);

    if (!Translation.HasValue())
    {
        PrintDebugError(
            L"TranslateX64VirtualAddress failed",
            Translation.Error);

        return false;
    }

    Formatter->PrintTranslation(Translation.Value);

    const auto Analysis = VSGDBCore::AnalyzeX64PageFault(
        Paging,
        Translation.Value,
        ErrorCode.Value);

    Formatter->PrintPageFaultAnalysis(Analysis);

    return true;
}

bool
CommandProcessor::ExecuteStep(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 1)
    {
        std::wprintf(L"Usage: step\n");
        return false;
    }

    VSGDBCore::DebugError Error = Session->StepInto(CurrentCpuId);
    if (!Error.IsSuccess())
    {
        PrintDebugError(L"Failed to step", Error);
        return false;
    }

    auto SessionState = Session->GetState();
    if (SessionState == VSGDBCore::DebugSessionState::Running ||
        SessionState == VSGDBCore::DebugSessionState::StopPending)
    {
        ScopedTargetRunning RunningGuard(
            TargetRunning,
            BreakRequested);

        auto Event = Session->WaitForEvent(INFINITE);
        if (!Event.HasValue())
        {
            PrintDebugError(L"Failed to wait for debug event", Event.Error);
            return false;
        }

        TryUpdateCurrentCpuFromStopEvent(Event.Value);
        ReportStopReason(Event.Value);
    }

    return true;
}

bool
CommandProcessor::ExecuteGo(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 1)
    {
        std::wprintf(L"Usage: g\n");
        return false;
    }

    ScopedTargetRunning RunningGuard(
        TargetRunning,
        BreakRequested);

    VSGDBCore::DebugError Error = Session->Continue(CurrentCpuId);
    if (!Error.IsSuccess())
    {
        PrintDebugError(L"Failed to continue", Error);
        return false;
    }

    auto SessionState = Session->GetState();
    if (SessionState == VSGDBCore::DebugSessionState::Running ||
        SessionState == VSGDBCore::DebugSessionState::StopPending)
    {
        auto Event = Session->WaitForEvent(INFINITE);
        if (!Event.HasValue())
        {
            PrintDebugError(L"Failed to wait for debug event", Event.Error);
            return false;
        }

        TryUpdateCurrentCpuFromStopEvent(Event.Value);
        ReportStopReason(Event.Value);
    }

    return true;
//    return ContinueAllAndReportStop();
}

bool
CommandProcessor::ExecuteBreakpointSet(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: bp <expr>\n");
        return false;
    }

    auto Address = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate breakpoint address",
            Address.Error);

        return false;
    }

    auto Breakpoint = Session->SetBreakpoint(
        VSGDBCore::BreakpointKind::Software,
        Address.Value,
        1);

    if (!Breakpoint.HasValue())
    {
        PrintDebugError(
            L"Failed to set breakpoint",
            Breakpoint.Error);

        return false;
    }

    std::wprintf(
        L"Breakpoint %u set at %s\n",
        Breakpoint.Value.Id,
        Formatter->FormatAddressInline(
            Breakpoint.Value.Address).c_str());

    return true;
}

bool
CommandProcessor::ExecuteBreakpointList(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 1)
    {
        std::wprintf(L"Usage: bl\n");
        return false;
    }

    bool Any = false;

    std::wprintf(
        L"Id  Address             Size  Kind       State     Symbol\n");

    auto Breakpoints = Session->EnumerateBreakpoints();

    if (!Breakpoints.HasValue())
    {
        PrintDebugError(
            L"Failed to enumerate breakpoints",
            Breakpoints.Error);

        return false;
    }

    for (const auto& Breakpoint : Breakpoints.Value)
    {
        if (!Breakpoint.Enabled)
        {
            continue;
        }

        Any = true;

        AddressLabel Label = Formatter->FormatAddressLabel(Breakpoint.Address);

        std::wprintf(
            L"%-3u 0x%016llx  %-4u %-10s %-8s",
            Breakpoint.Id,
            Breakpoint.Address,
            Breakpoint.Size,
            BreakpointKindName(Breakpoint.Kind),
            L"enabled");

        if (!Label.Text.empty())
        {
            std::wprintf(
                L" %s",
                Label.Text.c_str());
        }

        std::wprintf(L"\n");
    }

    if (!Any)
    {
        std::wprintf(L"No breakpoints.\n");
    }

    return true;
}

bool
CommandProcessor::ExecuteBreakpointClear(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: bc <id>\n");
        return false;
    }

    if (Arguments[1] == L"*")
    {
        ClearAllKnownBreakpoints();
        std::wprintf(L"All known breakpoints cleared.\n");
        return true;
    }

    VSGDBCore::U32 Id32 = 0;
    if (!ParseU32(Arguments[1], Id32))
    {
        std::wprintf(
            L"Invalid breakpoint ID: %s\n",
            Arguments[1].c_str());

        return false;
    }

    const auto BreakpointId =
        static_cast<VSGDBCore::BreakpointId>(Id32);

    VSGDBCore::DebugError Error =
        Session->DeleteBreakpoint(BreakpointId);

    if (!Error.IsSuccess())
    {
        PrintDebugError(
            L"Failed to clear breakpoint",
            Error);

        return false;
    }

    std::wprintf(
        L"Breakpoint %u cleared.\n",
        BreakpointId);

    return true;
}

bool
CommandProcessor::TryUpdateCurrentCpuFromStopEvent(
    const VSGDBCore::DebugEvent& Event)
{
    if (Event.RemoteThreadId.empty())
    {
        return false;
    }

    auto Threads = Session->EnumerateThreads();
    if (!Threads.HasValue())
    {
        return false;
    }

    for (const auto& Thread : Threads.Value)
    {
        if (AreSameRemoteThreadId(
            Thread.RemoteThreadId,
            Event.RemoteThreadId))
        {
            CurrentCpuId = Thread.CpuId;
            return true;
        }
    }

    return false;
}

void
CommandProcessor::ReportStopReason(
    const VSGDBCore::DebugEvent& Event)
{
    //
    // For now, only identify software breakpoint hits.
    // QEMU x86-64 gdbstub reports RIP == breakpoint address.
    //
    if (Event.RemoteThreadId.empty())
    {
        return;
    }

    auto Threads = Session->EnumerateThreads();
    if (!Threads.HasValue())
    {
        return;
    }

    bool FoundCpu = false;
    VSGDBCore::U32 StoppedCpuId = 0;

    for (const auto& Thread : Threads.Value)
    {
        if (AreSameRemoteThreadId(
            Thread.RemoteThreadId,
            Event.RemoteThreadId))
        {
            StoppedCpuId = Thread.CpuId;
            FoundCpu = true;
            break;
        }
    }

    if (!FoundCpu)
    {
        return;
    }

    auto Registers = Session->GetRegisters(StoppedCpuId);
    if (!Registers.HasValue())
    {
        return;
    }

    const VSGDBCore::U64 Rip = Registers.Value.Rip;
    auto FormattedAddress = Formatter->FormatAddressInline(Rip);

    if (Event.StopReason == VSGDBCore::DebugStopReason::Breakpoint)
    {
        // 
        // Find the matching breakpoint.
        // 

        bool BreakpointFound = false;
        VSGDBCore::U32 BreakpointId = 0;

        auto Breakpoints = Session->EnumerateBreakpoints();
        if (!Breakpoints.HasValue())
        {
            std::wprintf(
                L"Warning: Cannot enumerate breakpoints\n");
        }
        else
        {
            auto Iterator = std::find_if(
                Breakpoints.Value.begin(),
                Breakpoints.Value.end(),
                [Rip](const VSGDBCore::BreakpointInfo& bp)
                {
                    return
                        bp.Enabled &&
                        bp.Kind == VSGDBCore::BreakpointKind::Software &&
                        bp.Size == 1 &&
                        bp.Address == Rip;
                });

            if (Iterator != Breakpoints.Value.end())
            {
                BreakpointId = Iterator->Id;
                BreakpointFound = true;
            }
        }

        if (BreakpointFound)
        {
            std::wprintf(
                L"Hit breakpoint %u at %s\n",
                BreakpointId,
                FormattedAddress.c_str());
        }
        else
        {
            std::wprintf(
                L"Hit unknown breakpoint at %s\n",
                FormattedAddress.c_str());
        }
    }
    else if (Event.StopReason == VSGDBCore::DebugStopReason::Signal)
    {
        std::wprintf(
            L"Break at %s by signal\n",
            FormattedAddress.c_str());
    }
    else if (Event.StopReason == VSGDBCore::DebugStopReason::Unknown)
    {
        std::wprintf(
            L"Break at %s by unknown reason\n",
            FormattedAddress.c_str());
    }

    //
    // Optional generic fallback. Useful when stop was SIGTRAP but not ours,
    // for example single-step, external debug exception, or unknown trap.
    //
    // If DebugEvent has SignalNumber, use that instead of parsing Description.
    //
}

void
CommandProcessor::ClearAllKnownBreakpoints()
{
    VSGDBCore::DebugError Error =
        Session->DeleteAllBreakpoints();

    if (!Error.IsSuccess())
    {
        PrintDebugError(
            L"Failed to clear breakpoints",
            Error);
        //return false;
    }
}

bool
CommandProcessor::ExecuteBreakpointClearAddress(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: bcaddr <expr>\n");
        return false;
    }

    auto Address = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate breakpoint address",
            Address.Error);

        return false;
    }

    auto Error = Session->DeleteBreakpointByAddress(
        VSGDBCore::BreakpointKind::Software,
        Address.Value,
        1);

    if (!Error.IsSuccess())
    {
        PrintDebugError(
            L"Failed to clear breakpoint by address",
            Error);

        return false;
    }

    std::wprintf(
        L"Software breakpoint at 0x%016llx cleared.\n",
        Address.Value);

    return true;
}

bool
CommandProcessor::ContinueAllAndReportStop()
{
    BreakRequested.store(false, std::memory_order_release);
    TargetRunning.store(true, std::memory_order_release);

    auto Error = Session->ContinueAll();

    TargetRunning.store(false, std::memory_order_release);

    const bool WasBreakRequested =
        BreakRequested.load(std::memory_order_acquire);

    if (!Error.IsSuccess())
    {
        PrintDebugError(L"ContinueAll failed", Error);
        return false;
    }

    auto Event = Session->GetLastEvent();
    if (Event.HasValue())
    {
        PrintStopEvent(Event.Value);

        if (!WasBreakRequested)
        {
            TryUpdateCurrentCpuFromStopEvent(Event.Value);
        }

        ReportStopReason(Event.Value);
    }

    return true;
}

bool
CommandProcessor::ExecuteDisassemble(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() > 3)
    {
        std::wprintf(L"Usage: u [expr] [bytes]\n");
        return false;
    }

    if (!Disassembler)
    {
        std::wprintf(L"No disassembler is available.\n");
        return false;
    }

    std::wstring AddressExpression = L"rip";

    if (Arguments.size() >= 2)
    {
        AddressExpression = Arguments[1];
    }

    auto Address = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        AddressExpression,
        SymbolManager.get());

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate address",
            Address.Error);

        return false;
    }

    VSGDBCore::U64 ByteCount64 = DefaultDisassemblyBytes;

    if (Arguments.size() == 3)
    {
        auto EvalResult = EvaluateSimpleExpression(
            *Session,
            CurrentCpuId,
            Arguments[2],
            nullptr);

        if (!EvalResult.HasValue() ||
            EvalResult.Value == 0 ||
            EvalResult.Value > MaxDisassemblyBytes)
        {
            std::wprintf(
                L"Invalid byte count: %s\n",
                Arguments[2].c_str());

            return false;
        }

        ByteCount64 = EvalResult.Value;
    }

    const VSGDBCore::U32 ByteCount =
        static_cast<VSGDBCore::U32>(ByteCount64);

    const VSGDBCore::U64 StartAddress = Address.Value;

    if (StartAddress > UINT64_MAX - ByteCount)
    {
        std::wprintf(L"Disassembly range overflows.\n");
        return false;
    }

    const VSGDBCore::U64 EndAddress =
        StartAddress + ByteCount;

    VSGDBCore::U64 CurrentRip = 0;

    auto Registers = Session->GetRegisters(CurrentCpuId);

    if (Registers.HasValue())
    {
        CurrentRip = Registers.Value.Rip;
    }

    return DisassembleRange(
        StartAddress,
        ByteCount,
        CurrentRip,
        StartAddress,
        EndAddress);
}

bool
CommandProcessor::ExecuteDisassembleFunction(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() > 2)
    {
        std::wprintf(L"Usage: uf [expr]\n");
        return false;
    }

    auto Registers = Session->GetRegisters(CurrentCpuId);

    if (!Registers.HasValue())
    {
        PrintDebugError(L"Failed to read registers", Registers.Error);
        return false;
    }

    VSGDBCore::U64 Address = Registers.Value.Rip;

    if (Arguments.size() == 2)
    {
        auto Evaluated = EvaluateSimpleExpression(
            *Session,
            CurrentCpuId,
            Arguments[1],
            SymbolManager.get());

        if (!Evaluated.HasValue())
        {
            PrintDebugError(L"Failed to evaluate address", Evaluated.Error);
            return false;
        }

        Address = Evaluated.Value;
    }

    if (!SymbolManager)
    {
        std::wprintf(L"No symbol manager is available.\n");
        return false;
    }

    auto Symbol = SymbolManager->GetSymbolByAddress(Address);

    if (!Symbol.HasValue())
    {
        PrintDebugError(L"Failed to find containing function", Symbol.Error);
        return false;
    }

    VSGDBCore::U32 ByteCount = Symbol.Value.Size;

    if (ByteCount == 0)
    {
        ByteCount = DefaultFunctionDisassemblyBytes;
    }

    if (ByteCount > MaxFunctionDisassemblyBytes)
    {
        ByteCount = MaxFunctionDisassemblyBytes;
    }

    VSGDBCore::U64 VisibleStart = Symbol.Value.Address;
    VSGDBCore::U64 VisibleEnd = Symbol.Value.Address + Symbol.Value.Size;

    return DisassembleRange(
        Symbol.Value.Address,
        ByteCount,
        Registers.Value.Rip,
        VisibleStart,
        VisibleEnd);
}

bool
CommandProcessor::ExecuteDisassembleBackwardWindow(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() > 3)
    {
        std::wprintf(L"Usage: ub [expr] [bytes]\n");
        return false;
    }

    auto Registers = Session->GetRegisters(CurrentCpuId);

    if (!Registers.HasValue())
    {
        PrintDebugError(L"Failed to read registers", Registers.Error);
        return false;
    }

    VSGDBCore::U64 TargetAddress = Registers.Value.Rip;
    VSGDBCore::U32 WindowBytes = 0x80;

    if (Arguments.size() >= 2)
    {
        auto Evaluated = EvaluateSimpleExpression(
            *Session,
            CurrentCpuId,
            Arguments[1],
            SymbolManager.get());

        if (!Evaluated.HasValue())
        {
            PrintDebugError(L"Failed to evaluate address", Evaluated.Error);
            return false;
        }

        TargetAddress = Evaluated.Value;
    }

    if (Arguments.size() >= 3)
    {
        auto EvaluatedSize = EvaluateSimpleExpression(
            *Session,
            CurrentCpuId,
            Arguments[2],
            SymbolManager.get());

        if (!EvaluatedSize.HasValue())
        {
            PrintDebugError(L"Failed to evaluate byte count", EvaluatedSize.Error);
            return false;
        }

        if (EvaluatedSize.Value == 0 ||
            EvaluatedSize.Value > 0x1000)
        {
            std::wprintf(L"Invalid byte count.\n");
            return false;
        }

        WindowBytes = static_cast<VSGDBCore::U32>(EvaluatedSize.Value);
    }

    auto Symbol = SymbolManager->GetSymbolByAddress(TargetAddress);

    if (!Symbol.HasValue())
    {
        PrintDebugError(L"Failed to find containing function", Symbol.Error);
        return false;
    }

    const VSGDBCore::U64 FunctionStart = Symbol.Value.Address;
    const VSGDBCore::U32 FunctionSize =
        Symbol.Value.Size != 0 ? Symbol.Value.Size : 0x400;

    const VSGDBCore::U64 FunctionEnd =
        FunctionStart + FunctionSize;

    VSGDBCore::U64 VisibleStart =
        TargetAddress > WindowBytes / 2
        ? TargetAddress - WindowBytes / 2
        : FunctionStart;

    if (VisibleStart < FunctionStart)
    {
        VisibleStart = FunctionStart;
    }

    VSGDBCore::U64 VisibleEnd =
        TargetAddress + WindowBytes / 2;

    if (VisibleEnd > FunctionEnd)
    {
        VisibleEnd = FunctionEnd;
    }

    const VSGDBCore::U32 DecodeBytes =
        static_cast<VSGDBCore::U32>(VisibleEnd - FunctionStart);

    return DisassembleRange(
        FunctionStart,
        DecodeBytes,
        Registers.Value.Rip,
        VisibleStart,
        VisibleEnd);
}

bool
CommandProcessor::ExecuteListModules(
    const std::vector<std::wstring>& Arguments)
{
    UNREFERENCED_PARAMETER(Arguments);

    const auto Modules = ModuleManager->EnumerateModules();

    if (Modules.empty())
    {
        std::wprintf(L"No modules.\n");
        return true;
    }

    std::wprintf(
        L"Id   Start              End                Module\n");

    for (const auto& Module : Modules)
    {
        if (Module.Size != 0)
        {
            const VSGDBCore::U64 EndAddress =
                Module.BaseAddress + Module.Size;

            std::wprintf(
                L"%-4u %016llx   %016llx   %s\n",
                Module.Id,
                Module.BaseAddress,
                EndAddress,
                Module.Name.c_str());
        }
        else
        {
            std::wprintf(
                L"%-4u %016llx   ?                  %s\n",
                Module.Id,
                Module.BaseAddress,
                Module.Name.c_str());
        }
    }

    return true;
}

bool
CommandProcessor::ExecuteReload(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() < 3 || Arguments.size() > 4)
    {
        std::wprintf(L"Usage: .reload <image-path> <base> [size]\n");
        return false;
    }

    const std::wstring& ImagePath = Arguments[1];

    auto Base = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[2],
        SymbolManager.get());

    if (!Base.HasValue())
    {
        PrintDebugError(L"Invalid base address", Base.Error);
        return false;
    }

    VSGDBCore::U64 Size = 0;

    if (Arguments.size() == 4)
    {
        auto SizeResult = EvaluateSimpleExpression(
            *Session,
            CurrentCpuId,
            Arguments[3],
            nullptr);

        if (!SizeResult.HasValue() || SizeResult.Value == 0)
        {
            std::wprintf(
                L"Invalid module size: %s\n",
                Arguments[3].c_str());

            return false;
        }

        Size = SizeResult.Value;
    }
    else
    {
        auto ImageSize = ReadPeSizeOfImage(ImagePath);

        if (!ImageSize.HasValue())
        {
            PrintDebugError(
                L"Failed to infer PE image size",
                ImageSize.Error);

            return false;
        }

        Size = ImageSize.Value;
    }

    VSGDBCore::ModuleInfo Module{};
    Module.Name = GetFileNameFromPath(ImagePath);
    Module.ImagePath = ImagePath;
    Module.BaseAddress = Base.Value;
    Module.Size = Size;

    auto ModuleId = ModuleManager->AddModule(Module);

    if (!ModuleId.HasValue())
    {
        PrintDebugError(L"Failed to add module", ModuleId.Error);
        return false;
    }

    if (Arguments.size() == 4)
    {
        std::wprintf(
            L"Added module %s, Id=%u, Base=0x%016llx, Size=0x%llx\n",
            Module.Name.c_str(),
            ModuleId.Value,
            Module.BaseAddress,
            Module.Size);
    }
    else
    {
        std::wprintf(
            L"Added module %s, Id=%u, Base=0x%016llx, Size=0x%llx (PE SizeOfImage)\n",
            Module.Name.c_str(),
            ModuleId.Value,
            Module.BaseAddress,
            Module.Size);
    }

    return true;
}

bool
CommandProcessor::ExecuteSymbolLoad(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: .symload <module-id>\n");
        return false;
    }

    VSGDBCore::U64 ModuleIdValue = 0;

    auto ParsedId = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        Arguments[1],
        SymbolManager.get());

    if (!ParsedId.HasValue())
    {
        PrintDebugError(L"Invalid module ID", ParsedId.Error);
        return false;
    }

    ModuleIdValue = ParsedId.Value;

    if (ModuleIdValue == 0 ||
        ModuleIdValue > 0xffffffffull)
    {
        std::wprintf(L"Invalid module ID.\n");
        return false;
    }

    auto Module = ModuleManager->GetModuleById(
        static_cast<VSGDBCore::ModuleId>(ModuleIdValue));

    if (!Module.HasValue())
    {
        PrintDebugError(L"Failed to find module", Module.Error);
        return false;
    }

    VSGDBCore::DebugError Error =
        SymbolManager->LoadSymbolsForModule(Module.Value);

    if (!Error.IsSuccess())
    {
        PrintDebugError(L"Failed to load symbols", Error);
        return false;
    }

    std::wprintf(
        L"Loaded symbols for %s.\n",
        Module.Value.Name.c_str());

    return true;
}

bool
CommandProcessor::ExecuteSymbolLookup(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: x <symbol-name>\n");
        return false;
    }

    if (!SymbolManager)
    {
        std::wprintf(L"No symbol manager is available.\n");
        return false;
    }

    auto Symbol = SymbolManager->GetSymbolByName(
        Arguments[1]);

    if (!Symbol.HasValue())
    {
        PrintDebugError(
            L"Failed to find symbol",
            Symbol.Error);

        return false;
    }

    std::wstring ModuleName = L"?";

    if (ModuleManager)
    {
        auto Module = ModuleManager->GetModuleById(
            Symbol.Value.ModuleId);

        if (Module.HasValue())
        {
            if (!Module.Value.Name.empty())
            {
                ModuleName = Module.Value.Name;
            }
            else if (!Module.Value.ImagePath.empty())
            {
                ModuleName = Module.Value.ImagePath;
            }
        }
    }

    std::wprintf(
        L"0x%016llx  %s",
        Symbol.Value.Address,
        Symbol.Value.Name.c_str());

    if (Symbol.Value.Size != 0)
    {
        std::wprintf(
            L"  Size=0x%x",
            Symbol.Value.Size);
    }

    std::wprintf(
        L"  Module=%s\n",
        ModuleName.c_str());

    return true;
}

bool
CommandProcessor::DisassembleRange(
    VSGDBCore::U64 StartAddress,
    VSGDBCore::U32 ByteCount,
    VSGDBCore::U64 CurrentRip,
    VSGDBCore::U64 VisibleStart,
    VSGDBCore::U64 VisibleEnd)
{
    static constexpr VSGDBCore::U32 MaxInstructionLength = 15;

    const VSGDBCore::U32 ReadSize =
        ByteCount + MaxInstructionLength;

    auto Bytes = Session->ReadVirtualMemory(
        CurrentCpuId,
        StartAddress,
        ReadSize);

    if (!Bytes.HasValue())
    {
        PrintDebugError(L"Failed to read memory", Bytes.Error);
        return false;
    }

    auto Instructions = Disassembler->Disassemble(
        StartAddress,
        Bytes.Value,
        4096);

    if (!Instructions.HasValue())
    {
        PrintDebugError(L"Failed to disassemble", Instructions.Error);
        return false;
    }

    std::vector<VSGDBCore::DisassembledInstruction> Visible;

    const VSGDBCore::U64 EndAddress =
        StartAddress + ByteCount;

    for (const auto& Instruction : Instructions.Value)
    {
        if (Instruction.Address >= StartAddress &&
            Instruction.Address < EndAddress)
        {
            if (Instruction.Address >= VisibleStart &&
                Instruction.Address < VisibleEnd)
            {
                Visible.push_back(Instruction);
            }
        }
    }

    PrintDisassembly(
        Visible,
        CurrentRip,
        [this](VSGDBCore::U64 Address)
        {
            return Formatter->FormatAddressLabel(Address);
        });

    return true;
}

bool
CommandProcessor::ExecuteSourceLine(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() > 2)
    {
        std::wprintf(L"Usage: line [expr]\n");
        return false;
    }

    if (!SymbolManager)
    {
        std::wprintf(L"No symbol manager is available.\n");
        return false;
    }

    std::wstring AddressExpression = L"rip";

    if (Arguments.size() == 2)
    {
        AddressExpression = Arguments[1];
    }

    auto Address = EvaluateSimpleExpression(
        *Session,
        CurrentCpuId,
        AddressExpression,
        SymbolManager.get());

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate address",
            Address.Error);

        return false;
    }

    auto Location =
        SymbolManager->GetSourceLocationByAddress(Address.Value);

    if (!Location.HasValue())
    {
        PrintDebugError(
            L"Failed to find source line",
            Location.Error);

        return false;
    }

    std::wprintf(
        L"%s\n",
        Formatter->FormatAddressInline(Address.Value).c_str());

    if (Location.Value.Column != 0)
    {
        std::wprintf(
            L"%s:%u:%u\n",
            Location.Value.FilePath.c_str(),
            Location.Value.Line,
            Location.Value.Column);
    }
    else
    {
        std::wprintf(
            L"%s:%u\n",
            Location.Value.FilePath.c_str(),
            Location.Value.Line);
    }

    return true;
}

bool
CommandProcessor::ExecuteSymbol(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 2)
    {
        std::wprintf(L"Usage: x <symbol-name|pattern>\n");
        return false;
    }

    const SymbolQuery Query =
        ParseSymbolQuery(Arguments[1]);

    if (IsSymbolEnumerationQuery(Query))
    {
        return ExecuteSymbolSearch(Query);
    }

    return ExecuteSymbolLookup(Arguments);
}

bool
CommandProcessor::ExecuteSymbolSearch(
    const SymbolQuery& Query)
{
    size_t Printed = 0;
    bool Truncated = false;

    const auto Modules =
        ModuleManager->EnumerateModules();

    for (const auto& Module : Modules)
    {
        auto Symbols =
            SymbolManager->EnumerateSymbols(Module.Id);

        if (!Symbols.HasValue())
        {
            continue;
        }

        for (const auto& Symbol : Symbols.Value)
        {
            if (!MatchesSymbolQuery(Symbol.Name, Query))
            {
                continue;
            }

            PrintSymbolInfo(Symbol, &Module);

            ++Printed;

            if (Printed >= MaxSymbolSearchResults)
            {
                Truncated = true;
                break;
            }
        }

        if (Truncated)
        {
            break;
        }
    }

    if (Printed == 0)
    {
        std::wprintf(L"No matching symbols.\n");
        return false;
    }

    if (Truncated)
    {
        std::wprintf(
            L"Results truncated after %zu symbols.\n",
            MaxSymbolSearchResults);
    }

    return true;
}

void
CommandProcessor::PrintSymbolInfo(
    const VSGDBCore::SymbolInfo& Symbol,
    const VSGDBCore::ModuleInfo* Module) const
{
    std::wprintf(
        L"0x%016llx  %-48s  %-6s",
        Symbol.Address,
        Symbol.Name.c_str(),
        SymbolKindToString(Symbol.Kind));

    if (Symbol.Size != 0)
    {
        std::wprintf(
            L"  Size=0x%x",
            Symbol.Size);
    }
    else
    {
        std::wprintf(
            L"  Size=?");
    }

    if (Module != nullptr)
    {
        std::wprintf(
            L"  Module=%s",
            Module->Name.c_str());
    }

    std::wprintf(L"\n");
}

bool
CommandProcessor::ExecuteStackTrace(
    const std::vector<std::wstring>& Arguments,
    bool Verbose)
{
    if (Arguments.size() > 2)
    {
        std::wprintf(L"Usage: %s [max-frames]\n",
            Verbose ? L"kb" : L"k");
        return false;
    }

    if (!StackWalker)
    {
        std::wprintf(L"No stack walker is available.\n");
        return false;
    }

    VSGDBCore::StackWalkOptions Options{};

    if (Arguments.size() == 2)
    {
        auto MaxFrames = EvaluateSimpleExpression(
            *Session,
            CurrentCpuId,
            Arguments[1],
            nullptr);

        if (!MaxFrames.HasValue() ||
            MaxFrames.Value == 0 ||
            MaxFrames.Value > 1024)
        {
            std::wprintf(
                L"Invalid max frame count: %s\n",
                Arguments[1].c_str());

            return false;
        }

        Options.MaxFrames =
            static_cast<VSGDBCore::U32>(MaxFrames.Value);
    }

    auto Frames = StackWalker->WalkStack(
        *Session,
        CurrentCpuId,
        Options);

    if (!Frames.HasValue())
    {
        PrintDebugError(
            L"Failed to walk stack",
            Frames.Error);

        return false;
    }

    for (const auto& Frame : Frames.Value)
    {
        if (!Verbose)
        {
            std::wprintf(
                L"#%-2u  %s\n",
                Frame.Index,
                Formatter->FormatAddressInline(
                    Frame.InstructionPointer).c_str());
        }
        else
        {
            std::wprintf(
                L"#%-2u  RIP=%s  RBP=0x%016llx  RSP=0x%016llx  "
                L"Establisher=0x%016llx  RetSlot=0x%016llx  RET=%s\n",
                Frame.Index,
                Formatter->FormatAddressInline(
                    Frame.InstructionPointer).c_str(),
                Frame.FramePointer,
                Frame.StackPointer,
                Frame.EstablisherFrame,
                Frame.ReturnAddressLocation,
                Formatter->FormatAddressInline(
                    Frame.ReturnAddress).c_str());
        }
    }

    return true;
}