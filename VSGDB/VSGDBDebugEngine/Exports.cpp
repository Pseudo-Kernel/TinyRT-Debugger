
#define INITGUID

#include "DebugEngine.h"
#include "DebugProgramNode.h"
#include "DebugEngineClassFactory.h"
#include "VSGDBDebugEngineGuids.h"
#include "SessionHost.h"
#include "LogUtils.h"


extern std::atomic<ULONG> g_DllObjectCount;
extern std::atomic<ULONG> g_DllLockCount;

extern "C"
int
VsgdbDebugEngineSmokeTest(
    const wchar_t* Host,
    unsigned short Port)
{
    if (Host == nullptr || Host[0] == L'\0')
    {
        Host = L"localhost";
    }

    SessionHost HostObject;

    VSGDBCore::DebugError Error =
        HostObject.Connect(
            Host,
            static_cast<VSGDBCore::U16>(Port ? Port : 1234));

    if (!Error.IsSuccess())
    {
        return static_cast<int>(Error.Code);
    }

    Error = HostObject.Disconnect();
    if (!Error.IsSuccess())
    {
        return static_cast<int>(Error.Code);
    }

    return 0;
}

extern "C" __declspec(dllexport)
int
VsgdbDebugProgramNodeSmokeTest()
{
    VsgdbLog(L"VsgdbDebugProgramNodeSmokeTest begin");

    DebugProgramNode* Node = new DebugProgramNode();

    BSTR ProgramName = nullptr;
    HRESULT Hr = Node->GetProgramName(&ProgramName);
    if (FAILED(Hr))
    {
        Node->Release();
        return static_cast<int>(Hr);
    }

    if (ProgramName != nullptr)
    {
        VsgdbLogFormat(
            L"ProgramName=%s",
            ProgramName);

        SysFreeString(ProgramName);
    }

    BSTR EngineName = nullptr;
    GUID EngineGuid = {};
    Hr = Node->GetEngineInfo(&EngineName, &EngineGuid);
    if (FAILED(Hr))
    {
        Node->Release();
        return static_cast<int>(Hr);
    }

    if (EngineName != nullptr)
    {
        VsgdbLogFormat(
            L"EngineName=%s",
            EngineName);

        SysFreeString(EngineName);
    }

    if (EngineGuid == GUID_VSGDBDebugEngine)
    {
        VsgdbLog(L"EngineGuid matched GUID_VSGDBDebugEngine");
    }
    else
    {
        VsgdbLog(L"EngineGuid mismatch");
        Node->Release();
        return -1;
    }

    Node->Release();

    VsgdbLog(L"VsgdbDebugProgramNodeSmokeTest end");

    return 0;
}

extern "C"
HRESULT STDMETHODCALLTYPE
DllGetClassObject(
    REFCLSID ClassId,
    REFIID InterfaceId,
    void** Object)
{
    VsgdbLog(__FUNCTIONW__);

    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (ClassId != CLSID_VSGDBDebugEngine)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    DebugEngineClassFactory* Factory =
        new (std::nothrow) DebugEngineClassFactory();

    if (Factory == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT Result =
        Factory->QueryInterface(
            InterfaceId,
            Object);

    Factory->Release();

    return Result;
}

extern "C"
HRESULT STDMETHODCALLTYPE
DllCanUnloadNow()
{
    if (g_DllObjectCount.load(std::memory_order_relaxed) == 0 &&
        g_DllLockCount.load(std::memory_order_relaxed) == 0)
    {
        return S_OK;
    }

    return S_FALSE;
}

