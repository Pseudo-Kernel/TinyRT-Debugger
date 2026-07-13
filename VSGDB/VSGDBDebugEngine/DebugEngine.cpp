
#include "DebugEngine.h"

#include "LogUtils.h"
#include "VSGDBDebugEngineGuids.h"

DebugEngine::DebugEngine()
    : ReferenceCount_(1)
{
    VsgdbLog(L"DebugEngine created");
}

HRESULT STDMETHODCALLTYPE
DebugEngine::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugEngine2))
    {
        *Object = static_cast<IDebugEngine2*>(this);
        AddRef();
        return S_OK;
    }

    if (InterfaceId == __uuidof(IDebugEngineLaunch2))
    {
        *Object = static_cast<IDebugEngineLaunch2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugEngine::AddRef()
{
    return ReferenceCount_.fetch_add(
        1,
        std::memory_order_relaxed) + 1;
}

ULONG STDMETHODCALLTYPE
DebugEngine::Release()
{
    ULONG NewReferenceCount =
        ReferenceCount_.fetch_sub(
            1,
            std::memory_order_acq_rel) - 1;

    if (NewReferenceCount == 0)
    {
        VsgdbLog(L"DebugEngine destroyed");
        delete this;
    }

    return NewReferenceCount;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::EnumPrograms(
    IEnumDebugPrograms2** Programs)
{
    VsgdbLog(L"DebugEngine::EnumPrograms");

    if (Programs == nullptr)
    {
        return E_POINTER;
    }

    *Programs = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::Attach(
    IDebugProgram2** Programs,
    IDebugProgramNode2** ProgramNodes,
    DWORD ProgramCount,
    IDebugEventCallback2* Callback,
    ATTACH_REASON Reason)
{
    UNREFERENCED_PARAMETER(Programs);
    UNREFERENCED_PARAMETER(ProgramNodes);
    UNREFERENCED_PARAMETER(ProgramCount);
    UNREFERENCED_PARAMETER(Callback);
    UNREFERENCED_PARAMETER(Reason);

    VsgdbLog(L"DebugEngine::Attach");

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::CreatePendingBreakpoint(
    IDebugBreakpointRequest2* BreakpointRequest,
    IDebugPendingBreakpoint2** PendingBreakpoint)
{
    UNREFERENCED_PARAMETER(BreakpointRequest);

    VsgdbLog(L"DebugEngine::CreatePendingBreakpoint");

    if (PendingBreakpoint == nullptr)
    {
        return E_POINTER;
    }

    *PendingBreakpoint = nullptr;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::SetException(
    EXCEPTION_INFO* Exception)
{
    UNREFERENCED_PARAMETER(Exception);

    VsgdbLog(L"DebugEngine::SetException");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::RemoveSetException(
    EXCEPTION_INFO* Exception)
{
    UNREFERENCED_PARAMETER(Exception);

    VsgdbLog(L"DebugEngine::RemoveSetException");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::RemoveAllSetExceptions(
    REFGUID GuidType)
{
    UNREFERENCED_PARAMETER(GuidType);

    VsgdbLog(L"DebugEngine::RemoveAllSetExceptions");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::GetEngineId(
    GUID* EngineId)
{
    VsgdbLog(L"DebugEngine::GetEngineId");

    if (EngineId == nullptr)
    {
        return E_POINTER;
    }

    *EngineId = GUID_VSGDBDebugEngine;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::DestroyProgram(
    IDebugProgram2* Program)
{
    UNREFERENCED_PARAMETER(Program);

    VsgdbLog(L"DebugEngine::DestroyProgram");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::ContinueFromSynchronousEvent(
    IDebugEvent2* Event)
{
    UNREFERENCED_PARAMETER(Event);

    VsgdbLog(L"DebugEngine::ContinueFromSynchronousEvent");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::SetLocale(
    WORD LangId)
{
    VsgdbLogFormat(L"DebugEngine::SetLocale: LangId 0x%hx", LangId);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::SetRegistryRoot(
    LPCOLESTR RegistryRoot)
{
    if (RegistryRoot != nullptr)
    {
        VsgdbLogFormat(
            L"DebugEngine::SetRegistryRoot: %s",
            RegistryRoot);
    }
    else
    {
        VsgdbLog(L"DebugEngine::SetRegistryRoot: null");
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::SetMetric(
    LPCOLESTR Metric,
    VARIANT Value)
{
    if (Metric != nullptr)
    {
        VsgdbLogFormat(
            L"DebugEngine::SetMetric: Metric %s",
            Metric);
    }
    else
    {
        VsgdbLog(L"DebugEngine::SetMetric: Metric null");
    }

    UNREFERENCED_PARAMETER(Value);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::CauseBreak()
{
    VsgdbLog(L"DebugEngine::CauseBreak");

    return E_NOTIMPL;
}

// 
// IDebugEngineLaunch2
//
// 

HRESULT STDMETHODCALLTYPE
DebugEngine::LaunchSuspended(
    LPCOLESTR Server,
    IDebugPort2* Port,
    LPCOLESTR Exe,
    LPCOLESTR Args,
    LPCOLESTR Dir,
    BSTR Env,
    LPCOLESTR Options,
    LAUNCH_FLAGS LaunchFlags,
    DWORD StandardInput,
    DWORD StandardOutput,
    DWORD StandardError,
    IDebugEventCallback2* Callback,
    IDebugProcess2** Process)
{
    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: Exe=%s Args=%s Dir=%s Options=%s",
        Exe != nullptr ? Exe : L"(null)",
        Args != nullptr ? Args : L"(null)",
        Dir != nullptr ? Dir : L"(null)",
        Options != nullptr ? Options : L"(null)");

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::ResumeProcess(
    IDebugProcess2 *Process)
{
    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: Process=%p",
        Process);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::CanTerminateProcess(
    IDebugProcess2 *Process)
{
    VsgdbLogFormat(
        L"DebugEngine::CanTerminateProcess: Process=%p",
        Process);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::TerminateProcess(
    IDebugProcess2 *Process)
{
    VsgdbLogFormat(
        L"DebugEngine::TerminateProcess: Process=%p",
        Process);

    return S_OK;
}

