#pragma once

#include <VSGDBCore/IDebugSession.h>
#include <VSGDBCore/IDisassembler.h>
#include <VSGDBCore/IModuleManager.h>
#include <VSGDBCore/ISymbolManager.h>
#include <VSGDBCore/IStackWalker.h>

#include "DebugTextFormatter.h"
#include "SymbolQuery.h"

#include <memory>
#include <string>
#include <vector>
#include <atomic>


class CommandProcessor final
{
public:
    explicit CommandProcessor(
        std::unique_ptr<VSGDBCore::IDebugSession> Session,
        std::unique_ptr<VSGDBCore::IDisassembler> Disassembler,
        std::unique_ptr<VSGDBCore::IModuleManager> ModuleManager,
        std::unique_ptr<VSGDBCore::ISymbolManager> SymbolManager,
        std::unique_ptr<VSGDBCore::IStackWalker> StackWalker);

    int Run();

    bool RequestBreakFromConsoleControlHandler();

    bool ProcessLine(
        const std::wstring& Line);

private:

    bool ExecuteCommand(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteHelp(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteScript(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteQuit(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteThreads(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteCpu(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteRegisters(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteDumpVirtualBytes(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteDumpPhysicalBytes(
        const std::vector<std::wstring>& Arguments);

    bool ExecutePte(
        const std::vector<std::wstring>& Arguments);

    bool ExecutePageFault(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteStep(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteGo(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteBreakpointSet(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteBreakpointList(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteBreakpointClear(
        const std::vector<std::wstring>& Arguments);

    bool
    TryUpdateCurrentCpuFromStopEvent(
        const VSGDBCore::DebugEvent& Event);

    void ReportStopReason(
        const VSGDBCore::DebugEvent& Event);

    void ClearAllKnownBreakpoints();

    bool ExecuteBreakpointClearAddress(
        const std::vector<std::wstring>& Arguments);

//    bool ExecuteGoFromBreakpoint(
//        const std::vector<std::wstring>& Arguments);

    bool ContinueAllAndReportStop();

//    const CliBreakpoint* FindEnabledSoftwareBreakpointByAddress(
//        VSGDBCore::U64 Address) const;

//    CliBreakpoint* FindEnabledSoftwareBreakpointByAddressMutable(
//        VSGDBCore::U64 Address);

    bool ExecuteDisassemble(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteDisassembleFunction(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteDisassembleBackwardWindow(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteListModules(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteReload(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteSymbolLoad(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteSymbolLookup(
        const std::vector<std::wstring>& Arguments);

    bool DisassembleRange(
        VSGDBCore::U64 DecodeStart,
        VSGDBCore::U32 DecodeBytes,
        VSGDBCore::U64 CurrentRip,
        VSGDBCore::U64 VisibleStart,
        VSGDBCore::U64 VisibleEnd);

    bool ExecuteSourceLine(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteSymbol(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteSymbolSearch(
        const SymbolQuery& Query);

    void PrintSymbolInfo(
        const VSGDBCore::SymbolInfo& Symbol,
        const VSGDBCore::ModuleInfo* Module) const;

    bool ExecuteStackTrace(
        const std::vector<std::wstring>& Arguments,
        bool Verbose);

private:
    std::unique_ptr<VSGDBCore::IDebugSession> Session_;
    VSGDBCore::U32 CurrentCpuId_ = 0;
    bool QuitRequested_ = false;

    std::atomic<bool> TargetRunning_ = false;
    std::atomic<bool> BreakRequested_ = false;

    std::unique_ptr<VSGDBCore::IDisassembler> Disassembler_;
    std::unique_ptr<VSGDBCore::IModuleManager> ModuleManager_;
    std::unique_ptr<VSGDBCore::ISymbolManager> SymbolManager_;
    std::unique_ptr<VSGDBCore::IStackWalker> StackWalker_;

    std::unique_ptr<DebugTextFormatter> Formatter_;

    VSGDBCore::U32 ScriptDepth_ = 0;
};
