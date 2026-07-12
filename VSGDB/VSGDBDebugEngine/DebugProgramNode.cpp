#include "DebugProgramNode.h"

#include "LogUtils.h"
#include "VSGDBDebugEngineGuids.h"

#include <processthreadsapi.h>

DebugProgramNode::DebugProgramNode()
{
    VsgdbLog(L"DebugProgramNode created");
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugProgramNode2))
    {
        *Object = static_cast<IDebugProgramNode2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugProgramNode::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
DebugProgramNode::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::GetProgramName(
    BSTR* ProgramName)
{
    VsgdbLog(L"DebugProgramNode::GetProgramName");

    if (ProgramName == nullptr)
    {
        return E_POINTER;
    }

    *ProgramName = SysAllocString(L"VSGDB Kernel");

    if (*ProgramName == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::GetHostName(
    DWORD HostNameType,
    BSTR* HostName)
{
    VsgdbLogFormat(
        L"DebugProgramNode::GetHostName: HostNameType=%lu",
        HostNameType);

    if (HostName == nullptr)
    {
        return E_POINTER;
    }

    *HostName = SysAllocString(L"localhost");

    if (*HostName == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::GetHostPid(
    AD_PROCESS_ID* HostProcessId)
{
    VsgdbLog(L"DebugProgramNode::GetHostPid");

    if (HostProcessId == nullptr)
    {
        return E_POINTER;
    }

    ZeroMemory(
        HostProcessId,
        sizeof(*HostProcessId));

    //
    // This is a synthetic kernel target. There is no real host process.
    // For now, use the hosting devenv.exe process as a harmless placeholder.
    //
    HostProcessId->ProcessIdType = AD_PROCESS_ID_SYSTEM;
    HostProcessId->ProcessId.dwProcessId = GetCurrentProcessId();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::GetHostMachineName_V7(
    BSTR* HostMachineName)
{
    VsgdbLog(L"DebugProgramNode::GetHostMachineName_V7");

    if (HostMachineName == nullptr)
    {
        return E_POINTER;
    }

    *HostMachineName = SysAllocString(L"localhost");

    if (*HostMachineName == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::Attach_V7(
    IDebugProgram2* Program,
    IDebugEventCallback2* Callback,
    DWORD Reason)
{
    VsgdbLogFormat(
        L"DebugProgramNode::Attach_V7: Program=%p Callback=%p Reason=%lu",
        Program,
        Callback,
        Reason);

    //
    // This is the legacy node-level attach hook. The real engine attach path
    // should be IDebugEngine2::Attach().
    //
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::GetEngineInfo(
    BSTR* EngineName,
    GUID* EngineGuid)
{
    VsgdbLog(L"DebugProgramNode::GetEngineInfo");

    if (EngineName == nullptr || EngineGuid == nullptr)
    {
        return E_POINTER;
    }

    *EngineName = SysAllocString(L"VSGDB Debug Engine");

    if (*EngineName == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    *EngineGuid = GUID_VSGDBDebugEngine;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgramNode::DetachDebugger_V7()
{
    VsgdbLog(L"DebugProgramNode::DetachDebugger_V7");

    return S_OK;
}

DebugProgramNode::~DebugProgramNode()
{
    VsgdbLog(L"DebugProgramNode destroyed");
}
