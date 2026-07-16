
#define INITGUID

#include "DebugEngine.h"
#include "DebugProgramNode.h"
#include "DebugProgram.h"
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

extern "C" __declspec(dllexport)
HRESULT __stdcall
VsgdbCreateDebugProgramNode(
    IDebugProgramNode2** ProgramNode)
{
    VsgdbLog(L"VsgdbCreateDebugProgramNode");

    if (ProgramNode == nullptr)
    {
        return E_POINTER;
    }

    *ProgramNode = nullptr;

    DebugProgramNode* Node = new (std::nothrow) DebugProgramNode();

    if (Node == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    *ProgramNode = static_cast<IDebugProgramNode2*>(Node);

    //
    // Node starts with ReferenceCount_ == 1.
    // Ownership is transferred to the caller.
    //

    return S_OK;
}

extern "C" __declspec(dllexport)
int __stdcall
VsgdbDebugProgramSmokeTest()
{
    VsgdbLog(L"VsgdbDebugProgramSmokeTest begin");

    DebugProgram* Program =
        new (std::nothrow) DebugProgram(nullptr);

    if (Program == nullptr)
    {
        return static_cast<int>(E_OUTOFMEMORY);
    }

    BSTR Name = nullptr;
    HRESULT Hr =
        Program->GetName(
            &Name);

    if (FAILED(Hr))
    {
        Program->Release();
        return static_cast<int>(Hr);
    }

    if (Name != nullptr)
    {
        VsgdbLogFormat(
            L"DebugProgram name=%s",
            Name);

        SysFreeString(Name);
    }

    GUID ProgramId = {};
    Hr =
        Program->GetProgramId(
            &ProgramId);

    if (FAILED(Hr))
    {
        Program->Release();
        return static_cast<int>(Hr);
    }

    BSTR EngineName = nullptr;
    GUID EngineGuid = {};
    Hr =
        Program->GetEngineInfo(
            &EngineName,
            &EngineGuid);

    if (FAILED(Hr))
    {
        Program->Release();
        return static_cast<int>(Hr);
    }

    if (EngineName != nullptr)
    {
        VsgdbLogFormat(
            L"DebugProgram engine=%s",
            EngineName);

        SysFreeString(EngineName);
    }

    if (EngineGuid != GUID_VSGDBDebugEngine)
    {
        Program->Release();
        return -1;
    }

    IEnumDebugThreads2* EnumThreads = nullptr;
    Hr =
        Program->EnumThreads(
            &EnumThreads);

    if (FAILED(Hr) || EnumThreads == nullptr)
    {
        Program->Release();
        return static_cast<int>(FAILED(Hr) ? Hr : E_FAIL);
    }

    ULONG ThreadCount = 0;
    Hr =
        EnumThreads->GetCount(
            &ThreadCount);

    VsgdbLogFormat(
        L"DebugProgram thread count=%lu Hr=0x%08x",
        ThreadCount,
        Hr);

    IDebugThread2* Thread = nullptr;
    ULONG Fetched = 0;

    Hr =
        EnumThreads->Next(
            1,
            &Thread,
            &Fetched);

    VsgdbLogFormat(
        L"DebugProgram first thread Hr=0x%08x Fetched=%lu Thread=%p",
        Hr,
        Fetched,
        Thread);

    if (Thread != nullptr)
    {
        BSTR ThreadName = nullptr;
        Hr =
            Thread->GetName(
                &ThreadName);

        if (SUCCEEDED(Hr) && ThreadName != nullptr)
        {
            VsgdbLogFormat(
                L"DebugThread name=%s",
                ThreadName);

            SysFreeString(ThreadName);
        }

        Thread->Release();
    }

    EnumThreads->Release();

    Program->Release();

    VsgdbLog(L"VsgdbDebugProgramSmokeTest end");

    return 0;
}

_Use_decl_annotations_
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

_Use_decl_annotations_
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

