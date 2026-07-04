
#include "DisassemblyPrinter.h"
#include "CommandProcessor.h"
#include "ExpressionUtil.h"
#include "RemoteThreadIdUtil.h"
#include "MemoryPrinter.h"
#include "PagingPrinter.h"
#include "PageFaultPrinter.h"

#include <VSGDBCore/X64Paging.h>
#include <cstdio>
#include <cwctype>
#include <sstream>
#include <iostream>
#include <Windows.h>

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
PrintRegister(
    const char* Name,
    VSGDBCore::U64 Value)
{
    std::printf(
        "%-8s = 0x%016llx\n",
        Name,
        Value);
}

static void
PrintRegisters(
    const VSGDBCore::RegisterContext& Context)
{
    PrintRegister("RAX", Context.Rax);
    PrintRegister("RBX", Context.Rbx);
    PrintRegister("RCX", Context.Rcx);
    PrintRegister("RDX", Context.Rdx);
    PrintRegister("RSI", Context.Rsi);
    PrintRegister("RDI", Context.Rdi);
    PrintRegister("RBP", Context.Rbp);
    PrintRegister("RSP", Context.Rsp);

    PrintRegister("R8", Context.R8);
    PrintRegister("R9", Context.R9);
    PrintRegister("R10", Context.R10);
    PrintRegister("R11", Context.R11);
    PrintRegister("R12", Context.R12);
    PrintRegister("R13", Context.R13);
    PrintRegister("R14", Context.R14);
    PrintRegister("R15", Context.R15);

    PrintRegister("RIP", Context.Rip);
    PrintRegister("RFLAGS", Context.Rflags);

    PrintRegister("CS", Context.Cs);
    PrintRegister("SS", Context.Ss);
    PrintRegister("DS", Context.Ds);
    PrintRegister("ES", Context.Es);
    PrintRegister("FS", Context.Fs);
    PrintRegister("GS", Context.Gs);

    PrintRegister("FSBASE", Context.FsBase);
    PrintRegister("GSBASE", Context.GsBase);
    PrintRegister("KGSBASE", Context.KernelGsBase);

    PrintRegister("CR0", Context.Cr0);
    PrintRegister("CR2", Context.Cr2);
    PrintRegister("CR3", Context.Cr3);
    PrintRegister("CR4", Context.Cr4);
    PrintRegister("CR8", Context.Cr8);
    PrintRegister("EFER", Context.Efer);
}

static bool
PrintRegisterByName(
    const VSGDBCore::RegisterContext& Context,
    const std::wstring& Name)
{
    const std::wstring LowerName = ToLower(Name);

    if (LowerName == L"rax") { PrintRegister("RAX", Context.Rax); return true; }
    if (LowerName == L"rbx") { PrintRegister("RBX", Context.Rbx); return true; }
    if (LowerName == L"rcx") { PrintRegister("RCX", Context.Rcx); return true; }
    if (LowerName == L"rdx") { PrintRegister("RDX", Context.Rdx); return true; }

    if (LowerName == L"rsi") { PrintRegister("RSI", Context.Rsi); return true; }
    if (LowerName == L"rdi") { PrintRegister("RDI", Context.Rdi); return true; }
    if (LowerName == L"rbp") { PrintRegister("RBP", Context.Rbp); return true; }
    if (LowerName == L"rsp") { PrintRegister("RSP", Context.Rsp); return true; }

    if (LowerName == L"r8") { PrintRegister("R8", Context.R8);  return true; }
    if (LowerName == L"r9") { PrintRegister("R9", Context.R9);  return true; }
    if (LowerName == L"r10") { PrintRegister("R10", Context.R10); return true; }
    if (LowerName == L"r11") { PrintRegister("R11", Context.R11); return true; }
    if (LowerName == L"r12") { PrintRegister("R12", Context.R12); return true; }
    if (LowerName == L"r13") { PrintRegister("R13", Context.R13); return true; }
    if (LowerName == L"r14") { PrintRegister("R14", Context.R14); return true; }
    if (LowerName == L"r15") { PrintRegister("R15", Context.R15); return true; }

    if (LowerName == L"rip") { PrintRegister("RIP", Context.Rip);    return true; }
    if (LowerName == L"rflags") { PrintRegister("RFLAGS", Context.Rflags); return true; }
    if (LowerName == L"eflags") { PrintRegister("RFLAGS", Context.Rflags); return true; }

    if (LowerName == L"cs") { PrintRegister("CS", Context.Cs); return true; }
    if (LowerName == L"ss") { PrintRegister("SS", Context.Ss); return true; }
    if (LowerName == L"ds") { PrintRegister("DS", Context.Ds); return true; }
    if (LowerName == L"es") { PrintRegister("ES", Context.Es); return true; }
    if (LowerName == L"fs") { PrintRegister("FS", Context.Fs); return true; }
    if (LowerName == L"gs") { PrintRegister("GS", Context.Gs); return true; }

    if (LowerName == L"fsbase") { PrintRegister("FSBASE", Context.FsBase);       return true; }
    if (LowerName == L"gsbase") { PrintRegister("GSBASE", Context.GsBase);       return true; }
    if (LowerName == L"kgsbase") { PrintRegister("KGSBASE", Context.KernelGsBase); return true; }

    if (LowerName == L"cr0") { PrintRegister("CR0", Context.Cr0);  return true; }
    if (LowerName == L"cr2") { PrintRegister("CR2", Context.Cr2);  return true; }
    if (LowerName == L"cr3") { PrintRegister("CR3", Context.Cr3);  return true; }
    if (LowerName == L"cr4") { PrintRegister("CR4", Context.Cr4);  return true; }
    if (LowerName == L"cr8") { PrintRegister("CR8", Context.Cr8);  return true; }
    if (LowerName == L"efer") { PrintRegister("EFER", Context.Efer); return true; }

    return false;
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



CommandProcessor::CommandProcessor(
    VSGDBCore::GdbRemoteTarget& Target,
    std::unique_ptr<VSGDBCore::IDisassembler> Disassembler,
    std::unique_ptr<VSGDBCore::IModuleManager> ModuleManager) :
    Target(Target),
    Disassembler(std::move(Disassembler)),
    ModuleManager(std::move(ModuleManager))
{
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
    auto Error = Target.BreakExecution();
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

    if (Command == L"lm")
    {
        return ExecuteListModules(Arguments);
    }

    if (Command == L".reload")
    {
        return ExecuteReload(Arguments);
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
        L"  lm                   List registered modules.\n"
        L"  .reload <image> <base> [size]   Register a module image at base address.\n"
    );

    return true;
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

    auto Threads = Target.EnumerateThreads();
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

    auto Threads = Target.EnumerateThreads();
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

    auto Registers = Target.GetRegisters(CurrentCpuId);
    if (!Registers.HasValue())
    {
        PrintDebugError(L"GetRegisters failed", Registers.Error);
        return false;
    }

    if (Arguments.size() == 1)
    {
        PrintRegisters(Registers.Value);
        return true;
    }

    bool AllValid = true;

    for (size_t Index = 1; Index < Arguments.size(); ++Index)
    {
        if (!PrintRegisterByName(
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
        Target,
        CurrentCpuId,
        Arguments[1]);

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

    auto Bytes = Target.ReadVirtualMemory(
        CurrentCpuId,
        Address.Value,
        static_cast<VSGDBCore::U32>(Size64));

    if (!Bytes.HasValue())
    {
        PrintDebugError(L"ReadVirtualMemory failed", Bytes.Error);
        return false;
    }

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
        Target,
        CurrentCpuId,
        Arguments[1]);

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

    auto Bytes = Target.ReadPhysicalMemory(
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
        Target,
        CurrentCpuId,
        Arguments[1]);

    if (!VirtualAddress.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate virtual address",
            VirtualAddress.Error);

        return false;
    }

    auto Registers = Target.GetRegisters(CurrentCpuId);
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
        Target,
        Paging,
        VirtualAddress.Value);

    if (!Translation.HasValue())
    {
        PrintDebugError(
            L"TranslateX64VirtualAddress failed",
            Translation.Error);

        return false;
    }

    PrintTranslation(Translation.Value);
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
        Target,
        CurrentCpuId,
        Arguments[1]);

    if (!VirtualAddress.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate virtual address",
            VirtualAddress.Error);

        return false;
    }

    auto ErrorCode = EvaluateSimpleExpression(
        Target,
        CurrentCpuId,
        Arguments[2]);

    if (!ErrorCode.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate page-fault error code:",
            ErrorCode.Error);

        return false;
    }

    auto Registers = Target.GetRegisters(CurrentCpuId);
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
        Target,
        Paging,
        VirtualAddress.Value);

    if (!Translation.HasValue())
    {
        PrintDebugError(
            L"TranslateX64VirtualAddress failed",
            Translation.Error);

        return false;
    }

    PrintTranslation(Translation.Value);

    const auto Analysis = VSGDBCore::AnalyzeX64PageFault(
        Paging,
        Translation.Value,
        ErrorCode.Value);

    PrintPageFaultAnalysis(Analysis);

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

    auto Error = Target.StepInto(CurrentCpuId);
    if (!Error.IsSuccess())
    {
        PrintDebugError(L"StepInto failed", Error);
        return false;
    }

    auto Event = Target.GetLastEvent();
    if (Event.HasValue())
    {
        PrintStopEvent(Event.Value);
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

    if (!StepOverCurrentSoftwareBreakpointIfNeeded())
    {
        return false;
    }

    return ContinueAllAndReportStop();
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
        Target,
        CurrentCpuId,
        Arguments[1]);

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate breakpoint address",
            Address.Error);

        return false;
    }

    for (const auto& Breakpoint : Breakpoints)
    {
        if (Breakpoint.Enabled &&
            Breakpoint.Kind == VSGDBCore::BreakpointKind::Software &&
            Breakpoint.Address == Address.Value)
        {
            std::wprintf(
                L"Breakpoint already exists at 0x%016llx. Id=%u\n",
                Address.Value,
                Breakpoint.Id);

            return false;
        }
    }

    auto SetResult = Target.SetBreakpoint(
        VSGDBCore::BreakpointKind::Software,
        Address.Value,
        1);

    if (!SetResult.HasValue())
    {
        PrintDebugError(L"SetBreakpoint failed", SetResult.Error);
        return false;
    }

    CliBreakpoint Breakpoint{};
    Breakpoint.Id = SetResult.Value;
    Breakpoint.Kind = VSGDBCore::BreakpointKind::Software;
    Breakpoint.Address = Address.Value;
    Breakpoint.Size = 1;
    Breakpoint.Enabled = true;

    Breakpoints.push_back(Breakpoint);

    std::wprintf(
        L"Breakpoint %u set at 0x%016llx\n",
        Breakpoint.Id,
        Breakpoint.Address);

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
        L"Id  Address             Size  Kind       State\n");

    for (const auto& Breakpoint : Breakpoints)
    {
        if (!Breakpoint.Enabled)
        {
            continue;
        }

        Any = true;

        std::wprintf(
            L"%-3u 0x%016llx  %-4u  %-10s enabled\n",
            Breakpoint.Id,
            Breakpoint.Address,
            Breakpoint.Size,
            BreakpointKindName(Breakpoint.Kind));
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

    for (auto& Breakpoint : Breakpoints)
    {
        if (Breakpoint.Id != BreakpointId)
        {
            continue;
        }

        if (!Breakpoint.Enabled)
        {
            std::wprintf(
                L"Breakpoint %u is already disabled.\n",
                BreakpointId);

            return false;
        }

        auto Error = Target.DeleteBreakpoint(BreakpointId);

        if (!Error.IsSuccess())
        {
            PrintDebugError(L"DeleteBreakpoint failed", Error);
            return false;
        }

        Breakpoint.Enabled = false;

        std::wprintf(
            L"Breakpoint %u cleared.\n",
            BreakpointId);

        return true;
    }

    std::wprintf(
        L"Breakpoint %u was not found.\n",
        BreakpointId);

    return false;
}

bool
CommandProcessor::TryUpdateCurrentCpuFromStopEvent(
    const VSGDBCore::DebugEvent& Event)
{
    if (Event.RemoteThreadId.empty())
    {
        return false;
    }

    auto Threads = Target.EnumerateThreads();
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

    auto Threads = Target.EnumerateThreads();
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

    auto Registers = Target.GetRegisters(StoppedCpuId);
    if (!Registers.HasValue())
    {
        return;
    }

    const VSGDBCore::U64 Rip = Registers.Value.Rip;

    const CliBreakpoint* Breakpoint =
        FindEnabledSoftwareBreakpointByAddress(Rip);

    if (Breakpoint != nullptr)
    {
        AddressLabel Label = FormatAddressWithModule(
            Breakpoint->Address);

        std::wprintf(
            L"Hit breakpoint %u at %s\n",
            Breakpoint->Id,
            Label.Text.c_str());

        return;
    }

    //
    // Optional generic fallback. Useful when stop was SIGTRAP but not ours,
    // for example single-step, external debug exception, or unknown trap.
    //
    // If DebugEvent has SignalNumber, use that instead of parsing Description.
    //
}

const CliBreakpoint*
CommandProcessor::FindEnabledSoftwareBreakpointByAddress(
    VSGDBCore::U64 Address) const
{
    for (const auto& Breakpoint : Breakpoints)
    {
        if (!Breakpoint.Enabled)
        {
            continue;
        }

        if (Breakpoint.Kind != VSGDBCore::BreakpointKind::Software)
        {
            continue;
        }

        if (Breakpoint.Address == Address)
        {
            return &Breakpoint;
        }
    }

    return nullptr;
}

void
CommandProcessor::ClearAllKnownBreakpoints()
{
    for (auto& Breakpoint : Breakpoints)
    {
        if (!Breakpoint.Enabled)
        {
            continue;
        }

        auto Error = Target.DeleteBreakpoint(Breakpoint.Id);
        if (!Error.IsSuccess())
        {
            std::wprintf(
                L"Warning: failed to clear breakpoint %u: %s\n",
                Breakpoint.Id,
                Error.Message.c_str());

            continue;
        }

        Breakpoint.Enabled = false;
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
        Target,
        CurrentCpuId,
        Arguments[1]);

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate breakpoint address",
            Address.Error);

        return false;
    }

    auto Error = Target.DeleteBreakpointByAddress(
        VSGDBCore::BreakpointKind::Software,
        Address.Value,
        1);

    if (!Error.IsSuccess())
    {
        PrintDebugError(L"DeleteBreakpointByAddress failed", Error);
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

    auto Error = Target.ContinueAll();

    TargetRunning.store(false, std::memory_order_release);

    const bool WasBreakRequested =
        BreakRequested.load(std::memory_order_acquire);

    if (!Error.IsSuccess())
    {
        PrintDebugError(L"ContinueAll failed", Error);
        return false;
    }

    auto Event = Target.GetLastEvent();
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

CliBreakpoint*
CommandProcessor::FindEnabledSoftwareBreakpointByAddressMutable(
    VSGDBCore::U64 Address)
{
    for (auto& Breakpoint : Breakpoints)
    {
        if (!Breakpoint.Enabled)
        {
            continue;
        }

        if (Breakpoint.Kind != VSGDBCore::BreakpointKind::Software)
        {
            continue;
        }

        if (Breakpoint.Address != Address)
        {
            continue;
        }

        if (Breakpoint.Size != 1)
        {
            continue;
        }

        return &Breakpoint;
    }

    return nullptr;
}

bool
CommandProcessor::ExecuteGoFromBreakpoint(
    const std::vector<std::wstring>& Arguments)
{
    if (Arguments.size() != 1)
    {
        std::wprintf(L"Usage: gb\n");
        return false;
    }

    auto Registers = Target.GetRegisters(CurrentCpuId);
    if (!Registers.HasValue())
    {
        PrintDebugError(L"GetRegisters failed", Registers.Error);
        return false;
    }

    const VSGDBCore::U64 Rip = Registers.Value.Rip;

    CliBreakpoint* Breakpoint =
        FindEnabledSoftwareBreakpointByAddressMutable(Rip);

    if (Breakpoint == nullptr)
    {
        return ContinueAllAndReportStop();
    }

    std::wprintf(
        L"Stepping over breakpoint %u at 0x%016llx\n",
        Breakpoint->Id,
        Breakpoint->Address);

    auto RemoveError = Target.RemoveBreakpointFromTarget(
        Breakpoint->Id);

    if (!RemoveError.IsSuccess())
    {
        PrintDebugError(L"RemoveBreakpointFromTarget failed", RemoveError);

        return false;
    }

    Breakpoint->Inserted = false;

    auto StepError = Target.StepInto(CurrentCpuId);
    if (!StepError.IsSuccess())
    {
        PrintDebugError(
            L"StepInto failed while stepping over breakpoint",
            StepError);

        //
        // Best-effort restore.
        //
        auto RestoreError = Target.InsertBreakpointIntoTarget(
            Breakpoint->Id);

        if (!RestoreError.IsSuccess())
        {
            std::wprintf(
                L"WARNING: failed to restore breakpoint %u: %s\n",
                Breakpoint->Id,
                RestoreError.Message.c_str());
        }
        else
        {
            Breakpoint->Inserted = true;
        }

        return false;
    }

    auto StepEvent = Target.GetLastEvent();
    if (StepEvent.HasValue())
    {
        PrintStopEvent(StepEvent.Value);
        TryUpdateCurrentCpuFromStopEvent(StepEvent.Value);
    }

    auto RestoreError = Target.InsertBreakpointIntoTarget(
        Breakpoint->Id);

    if (!RestoreError.IsSuccess())
    {
        PrintDebugError(
            L"InsertBreakpointIntoTarget failed while restoring breakpoint",
            RestoreError);

        Breakpoint->Inserted = false;
        return false;
    }

    Breakpoint->Inserted = true;

    std::wprintf(
        L"Breakpoint %u restored at 0x%016llx\n",
        Breakpoint->Id,
        Breakpoint->Address);

    return ContinueAllAndReportStop();
}

bool
CommandProcessor::StepOverCurrentSoftwareBreakpointIfNeeded()
{
    auto Registers = Target.GetRegisters(CurrentCpuId);
    if (!Registers.HasValue())
    {
        PrintDebugError(L"GetRegisters failed", Registers.Error);
        return false;
    }

    const VSGDBCore::U64 Rip = Registers.Value.Rip;

    CliBreakpoint* Breakpoint =
        FindEnabledSoftwareBreakpointByAddressMutable(Rip);

    if (Breakpoint == nullptr)
    {
        return true;
    }

    const VSGDBCore::BreakpointId BreakpointId = Breakpoint->Id;
    const VSGDBCore::U64 BreakpointAddress = Breakpoint->Address;

    //
    // Final UX: keep this short. It is useful enough to know that the debugger
    // handled the breakpoint resume path.
    //
    std::wprintf(
        L"Stepping over breakpoint %u at 0x%016llx\n",
        BreakpointId,
        BreakpointAddress);

    auto RemoveError = Target.RemoveBreakpointFromTarget(BreakpointId);
    if (!RemoveError.IsSuccess())
    {
        PrintDebugError(L"RemoveBreakpointFromTarget failed", RemoveError);
        return false;
    }

    Breakpoint->Inserted = false;

    auto StepError = Target.StepInto(CurrentCpuId);
    if (!StepError.IsSuccess())
    {
        PrintDebugError(
            L"StepInto failed while stepping over breakpoint",
            StepError);

        auto RestoreError = Target.InsertBreakpointIntoTarget(BreakpointId);
        if (!RestoreError.IsSuccess())
        {
            std::wprintf(
                L"WARNING: failed to restore breakpoint %u: %s\n",
                BreakpointId,
                RestoreError.Message.c_str());
        }
        else
        {
            Breakpoint->Inserted = true;
        }

        return false;
    }

    //
    // Do not print/report the internal single-step stop here.
    // It is an implementation detail of continuing from a software breakpoint.
    // However, update CurrentCpuId just in case QEMU reports the step stop
    // using a normalized/alternate thread ID.
    //
    auto StepEvent = Target.GetLastEvent();
    if (StepEvent.HasValue())
    {
        TryUpdateCurrentCpuFromStopEvent(StepEvent.Value);
    }

    auto RestoreError = Target.InsertBreakpointIntoTarget(BreakpointId);
    if (!RestoreError.IsSuccess())
    {
        PrintDebugError(
            L"InsertBreakpointIntoTarget failed while restoring breakpoint",
            RestoreError);

        Breakpoint->Inserted = false;
        return false;
    }

    Breakpoint->Inserted = true;

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

    std::wstring AddressExpression = L"rip";

    if (Arguments.size() >= 2)
    {
        AddressExpression = Arguments[1];
    }

    if (!Disassembler)
    {
        std::wprintf(L"No disassembler is available.\n");
        return false;
    }

    auto Address = EvaluateSimpleExpression(
        Target,
        CurrentCpuId,
        AddressExpression);

    if (!Address.HasValue())
    {
        PrintDebugError(
            L"Failed to evaluate address\n",
            Address.Error);

        return false;
    }

    VSGDBCore::U64 ByteCount64 = 0x80;

    if (Arguments.size() == 3)
    {
        if (!ParseU64(Arguments[2], ByteCount64) ||
            ByteCount64 == 0 ||
            ByteCount64 > 0x1000)
        {
            std::wprintf(
                L"Invalid byte count: %s\n",
                Arguments[2].c_str());

            return false;
        }
    }

    const VSGDBCore::U32 ByteCount =
        static_cast<VSGDBCore::U32>(ByteCount64);

    constexpr VSGDBCore::U32 MaxX64InstructionLength = 15;

    const VSGDBCore::U32 ReadSize =
        ByteCount + MaxX64InstructionLength;

    auto Bytes = Target.ReadVirtualMemory(
        CurrentCpuId,
        Address.Value,
        ReadSize);

    if (!Bytes.HasValue())
    {
        PrintDebugError(L"ReadVirtualMemory failed", Bytes.Error);
        return false;
    }

    const VSGDBCore::U32 MaxInstructionCount =
        ByteCount + MaxX64InstructionLength;

    auto Instructions = Disassembler->Disassemble(
        Address.Value,
        Bytes.Value,
        MaxInstructionCount);

    if (!Instructions.HasValue())
    {
        PrintDebugError(L"Disassemble failed", Instructions.Error);
        return false;
    }

    const VSGDBCore::U64 EndAddress =
        Address.Value + ByteCount;

    std::vector<VSGDBCore::DisassembledInstruction> VisibleInstructions;

    for (const auto& Instruction : Instructions.Value)
    {
        if (Instruction.Address >= EndAddress)
        {
            break;
        }

        VisibleInstructions.push_back(Instruction);
    }

    VSGDBCore::U64 CurrentRip = 0;

    auto Registers = Target.GetRegisters(CurrentCpuId);
    if (Registers.HasValue())
    {
        CurrentRip = Registers.Value.Rip;
    }

    PrintDisassembly(
        VisibleInstructions,
        CurrentRip,
        [this](VSGDBCore::U64 Address) -> AddressLabel
        {
            return FormatAddressWithModule(Address);
        });

    return true;
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
        Target,
        CurrentCpuId,
        Arguments[2]);

    if (!Base.HasValue())
    {
        PrintDebugError(L"Invalid base address", Base.Error);
        return false;
    }

    VSGDBCore::U64 Size = 0;

    if (Arguments.size() >= 4)
    {
        auto ParsedSize = EvaluateSimpleExpression(
            Target,
            CurrentCpuId,
            Arguments[3]);

        if (!ParsedSize.HasValue())
        {
            PrintDebugError(L"Invalid module size", ParsedSize.Error);
            return false;
        }

        Size = ParsedSize.Value;
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

    std::wprintf(
        L"Added module %s, Id=%u, Base=0x%016llx",
        Module.Name.c_str(),
        ModuleId.Value,
        Module.BaseAddress);

    if (Module.Size != 0)
    {
        std::wprintf(
            L", Size=0x%llx",
            Module.Size);
    }

    std::wprintf(L"\n");

    return true;
}

AddressLabel
CommandProcessor::FormatAddressWithModule(
    VSGDBCore::U64 Address) const
{
    AddressLabel Label{};

    if (!ModuleManager)
    {
        return Label;
    }

    auto Module = ModuleManager->GetModuleByAddress(Address);

    if (!Module.HasValue())
    {
        return Label;
    }

    const VSGDBCore::U64 Offset =
        Address - Module.Value.BaseAddress;

    wchar_t KeyBuffer[64] = {};
    std::swprintf(
        KeyBuffer,
        _countof(KeyBuffer),
        L"module:%u",
        Module.Value.Id);

    wchar_t TextBuffer[256] = {};
    std::swprintf(
        TextBuffer,
        _countof(TextBuffer),
        L"%s+0x%llx",
        Module.Value.Name.c_str(),
        Offset);

    Label.Key = KeyBuffer;
    Label.Text = TextBuffer;

    return Label;
}