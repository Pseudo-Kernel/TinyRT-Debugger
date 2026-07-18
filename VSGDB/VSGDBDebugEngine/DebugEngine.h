#pragma once

#include <Windows.h>
#include <Unknwn.h>
#include <OleAuto.h>
#include <msdbg.h>
#include <dbgmetric.h>

#include <atomic>

class DebugProgram;

class DebugEngine final :
    public IDebugEngine2,
    public IDebugEngineLaunch2
{
public:
    DebugEngine();

    ~DebugEngine();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE EnumPrograms(
        IEnumDebugPrograms2** Programs) override;

    HRESULT STDMETHODCALLTYPE Attach(
        IDebugProgram2** Programs,
        IDebugProgramNode2** ProgramNodes,
        DWORD ProgramCount,
        IDebugEventCallback2* Callback,
        ATTACH_REASON Reason) override;

    HRESULT STDMETHODCALLTYPE CreatePendingBreakpoint(
        IDebugBreakpointRequest2* BreakpointRequest,
        IDebugPendingBreakpoint2** PendingBreakpoint) override;

    HRESULT STDMETHODCALLTYPE SetException(
        EXCEPTION_INFO* Exception) override;

    HRESULT STDMETHODCALLTYPE RemoveSetException(
        EXCEPTION_INFO* Exception) override;

    HRESULT STDMETHODCALLTYPE RemoveAllSetExceptions(
        REFGUID GuidType) override;

    HRESULT STDMETHODCALLTYPE GetEngineId(
        GUID* EngineId) override;

    HRESULT STDMETHODCALLTYPE DestroyProgram(
        IDebugProgram2* Program) override;

    HRESULT STDMETHODCALLTYPE ContinueFromSynchronousEvent(
        IDebugEvent2* Event) override;

    HRESULT STDMETHODCALLTYPE SetLocale(
        WORD LangId) override;

    HRESULT STDMETHODCALLTYPE SetRegistryRoot(
        LPCOLESTR RegistryRoot) override;

    HRESULT STDMETHODCALLTYPE SetMetric(
        LPCOLESTR Metric,
        VARIANT Value) override;

    HRESULT STDMETHODCALLTYPE CauseBreak() override;


    // 
    // IDebugEngineLaunch2
    // 

    HRESULT STDMETHODCALLTYPE LaunchSuspended(
        LPCOLESTR pszServer,
        IDebugPort2 *pPort,
        LPCOLESTR pszExe,
        LPCOLESTR pszArgs,
        LPCOLESTR pszDir,
        BSTR bstrEnv,
        LPCOLESTR pszOptions,
        LAUNCH_FLAGS dwLaunchFlags,
        DWORD hStdInput,
        DWORD hStdOutput,
        DWORD hStdError,
        IDebugEventCallback2 *pCallback,
        IDebugProcess2 **ppProcess) override;

    HRESULT STDMETHODCALLTYPE ResumeProcess(
        IDebugProcess2 *pProcess) override;

    HRESULT STDMETHODCALLTYPE CanTerminateProcess(
        IDebugProcess2 *pProcess) override;

    HRESULT STDMETHODCALLTYPE TerminateProcess(
        IDebugProcess2 *pProcess) override;

private:
    HRESULT SendEvent(
        IDebugEvent2* Event, 
        REFIID EventInterfaceId, 
        DWORD Attributes, 
        IDebugProgram2* Program, 
        IDebugThread2* Thread);

    HRESULT SendEngineCreateEvent();

    HRESULT SendProgramCreateEvent();

    HRESULT SendThreadCreateEvent();

    HRESULT CaptureProgramFromProcess(
        IDebugProcess2* Process);

    void LogProgramInfo(
        IDebugProgram2* Program);


private:
    std::atomic<ULONG> ReferenceCount_ = 1;

private:
    DWORD LaunchedProcessId_ = 0;
    HANDLE LaunchedProcessHandle_ = nullptr;
    HANDLE LaunchedThreadHandle_ = nullptr;

private:
    IDebugEventCallback2* Callback_ = nullptr;
    IDebugProcess2* Process_ = nullptr;
    IDebugProgram2* Program_ = nullptr;

private:
    DebugProgram* VsgdbProgram_ = nullptr;
};
