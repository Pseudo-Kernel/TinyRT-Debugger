#pragma once

#include <VSGDBCore/GdbRemoteTarget.h>
#include <VSGDBCore/IDisassembler.h>
#include <VSGDBCore/IModuleManager.h>

#include <memory>
#include <string>
#include <vector>
#include <atomic>


struct CliBreakpoint
{
    VSGDBCore::BreakpointId Id = 0;
    VSGDBCore::BreakpointKind Kind = VSGDBCore::BreakpointKind::Software;
    VSGDBCore::U64 Address = 0;
    VSGDBCore::U32 Size = 1;
    bool Enabled = false;
    bool Inserted = false;
};

class CommandProcessor final
{
public:
    explicit CommandProcessor(
        VSGDBCore::GdbRemoteTarget& Target,
        std::unique_ptr<VSGDBCore::IDisassembler> Disassembler,
        std::unique_ptr<VSGDBCore::IModuleManager> ModuleManager);

    int Run();

    bool RequestBreakFromConsoleControlHandler();

private:
    bool ProcessLine(
        const std::wstring& Line);

    bool ExecuteCommand(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteHelp(
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

    bool ExecuteGoFromBreakpoint(
        const std::vector<std::wstring>& Arguments);

    bool ContinueAllAndReportStop();

    const CliBreakpoint* FindEnabledSoftwareBreakpointByAddress(
        VSGDBCore::U64 Address) const;

    CliBreakpoint* FindEnabledSoftwareBreakpointByAddressMutable(
        VSGDBCore::U64 Address);

    bool StepOverCurrentSoftwareBreakpointIfNeeded();

    bool ExecuteDisassemble(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteListModules(
        const std::vector<std::wstring>& Arguments);

    bool ExecuteReload(
        const std::vector<std::wstring>& Arguments);

    AddressLabel
        FormatAddressWithModule(
            VSGDBCore::U64 Address) const;

private:
    VSGDBCore::GdbRemoteTarget& Target;
    VSGDBCore::U32 CurrentCpuId = 0;
    bool QuitRequested = false;

    std::vector<CliBreakpoint> Breakpoints;

    std::atomic<bool> TargetRunning = false;
    std::atomic<bool> BreakRequested = false;

    std::unique_ptr<VSGDBCore::IDisassembler> Disassembler;
    std::unique_ptr<VSGDBCore::IModuleManager> ModuleManager;
};