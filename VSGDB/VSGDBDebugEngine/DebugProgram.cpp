#include "DebugProgram.h"
#include "DebugThread.h"
#include "EnumDebugThreads.h"

#include "LogUtils.h"
#include "VSGDBDebugEngineGuids.h"

#include <combaseapi.h>

DebugProgram::DebugProgram(
    IDebugProcess2* Process)
{
    CoCreateGuid(&ProgramId_);

    Process_ = Process;

    if (Process_ != nullptr)
    {
        Process_->AddRef();
    }

    MainThread_ = new (std::nothrow)
        DebugThread(this, 0);

    VsgdbLog(L"DebugProgram created");
}

DebugProgram::~DebugProgram()
{
    VsgdbLog(L"DebugProgram destroyed");

    if (MainThread_ != nullptr)
    {
        MainThread_->Release();
        MainThread_ = nullptr;
    }

    if (Process_ != nullptr)
    {
        Process_->Release();
        Process_ = nullptr;
    }
}

HRESULT STDMETHODCALLTYPE
DebugProgram::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugProgram2))
    {
        *Object = static_cast<IDebugProgram2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugProgram::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
DebugProgram::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::EnumThreads(
    IEnumDebugThreads2** Enum)
{
    VsgdbLog(L"DebugProgram::EnumThreads");

    if (Enum == nullptr)
    {
        return E_POINTER;
    }

    *Enum = nullptr;

    std::vector<IDebugThread2*> Threads;

    if (MainThread_ != nullptr)
    {
        Threads.push_back(
            static_cast<IDebugThread2*>(MainThread_));
    }

    EnumDebugThreads* Enumerator =
        new (std::nothrow) EnumDebugThreads(
            Threads);

    if (Enumerator == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    *Enum = Enumerator;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetName(
    BSTR* Name)
{
    VsgdbLog(L"DebugProgram::GetName");

    if (Name == nullptr)
    {
        return E_POINTER;
    }

    *Name = SysAllocString(L"VSGDB Kernel");

    if (*Name == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetProcess(
    IDebugProcess2** Process)
{
    VsgdbLog(L"DebugProgram::GetProcess");

    if (Process == nullptr)
    {
        return E_POINTER;
    }

    *Process = Process_;

    if (*Process != nullptr)
    {
        (*Process)->AddRef();
        return S_OK;
    }

    return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::Terminate()
{
    VsgdbLog(L"DebugProgram::Terminate: handled by DebugEngine::TerminateProcess");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::Attach(
    IDebugEventCallback2* Callback)
{
    VsgdbLogFormat(
        L"DebugProgram::Attach: Callback=%p",
        Callback);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::CanDetach()
{
    VsgdbLog(L"DebugProgram::CanDetach");

    //return S_OK;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::Detach()
{
    VsgdbLog(L"DebugProgram::Detach");

    //return S_OK;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetProgramId(
    GUID* ProgramId)
{
    VsgdbLog(L"DebugProgram::GetProgramId");

    if (ProgramId == nullptr)
    {
        return E_POINTER;
    }

    *ProgramId = ProgramId_;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetDebugProperty(
    IDebugProperty2** Property)
{
    VsgdbLog(L"DebugProgram::GetDebugProperty");

    if (Property == nullptr)
    {
        return E_POINTER;
    }

    *Property = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::Execute()
{
    VsgdbLog(L"DebugProgram::Execute");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::Continue(
    IDebugThread2* Thread)
{
    VsgdbLogFormat(
        L"DebugProgram::Continue: Thread=%p",
        Thread);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::Step(
    IDebugThread2* Thread,
    STEPKIND StepKind,
    STEPUNIT StepUnit)
{
    VsgdbLogFormat(
        L"DebugProgram::Step: Thread=%p StepKind=%lu StepUnit=%lu",
        Thread,
        StepKind,
        StepUnit);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::CauseBreak()
{
    VsgdbLog(L"DebugProgram::CauseBreak");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetEngineInfo(
    BSTR* EngineName,
    GUID* EngineGuid)
{
    VsgdbLog(L"DebugProgram::GetEngineInfo");

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
DebugProgram::EnumCodeContexts(
    IDebugDocumentPosition2* DocumentPosition,
    IEnumDebugCodeContexts2** Enum)
{
    VsgdbLogFormat(
        L"DebugProgram::EnumCodeContexts: DocumentPosition=%p",
        DocumentPosition);

    if (Enum == nullptr)
    {
        return E_POINTER;
    }

    *Enum = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetMemoryBytes(
    IDebugMemoryBytes2** MemoryBytes)
{
    VsgdbLog(L"DebugProgram::GetMemoryBytes");

    if (MemoryBytes == nullptr)
    {
        return E_POINTER;
    }

    *MemoryBytes = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetDisassemblyStream(
    DISASSEMBLY_STREAM_SCOPE Scope,
    IDebugCodeContext2* CodeContext,
    IDebugDisassemblyStream2** DisassemblyStream)
{
    VsgdbLogFormat(
        L"DebugProgram::GetDisassemblyStream: Scope=%lu CodeContext=%p",
        Scope,
        CodeContext);

    if (DisassemblyStream == nullptr)
    {
        return E_POINTER;
    }

    *DisassemblyStream = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::EnumModules(
    IEnumDebugModules2** Enum)
{
    VsgdbLog(L"DebugProgram::EnumModules");

    if (Enum == nullptr)
    {
        return E_POINTER;
    }

    *Enum = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::GetENCUpdate(
    IDebugENCUpdate** Update)
{
    VsgdbLog(L"DebugProgram::GetENCUpdate");

    if (Update == nullptr)
    {
        return E_POINTER;
    }

    *Update = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::EnumCodePaths(
    LPCOLESTR Hint,
    IDebugCodeContext2* Start,
    IDebugStackFrame2* Frame,
    BOOL Source,
    IEnumCodePaths2** Enum,
    IDebugCodeContext2** Safety)
{
    VsgdbLogFormat(
        L"DebugProgram::EnumCodePaths: Hint=%s Start=%p Frame=%p Source=%d",
        Hint != nullptr ? Hint : L"(null)",
        Start,
        Frame,
        Source);

    if (Enum == nullptr || Safety == nullptr)
    {
        return E_POINTER;
    }

    *Enum = nullptr;
    *Safety = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugProgram::WriteDump(
    DUMPTYPE DumpType,
    LPCOLESTR DumpUrl)
{
    VsgdbLogFormat(
        L"DebugProgram::WriteDump: DumpType=%lu DumpUrl=%s",
        DumpType,
        DumpUrl != nullptr ? DumpUrl : L"(null)");

    return E_NOTIMPL;
}

IDebugThread2*
DebugProgram::GetMainThreadForEvent()
{
    VsgdbLog(L"DebugProgram::GetMainThreadForEvent");

    if (MainThread_ == nullptr)
    {
        return nullptr;
    }

    IDebugThread2* Thread =
        static_cast<IDebugThread2*>(MainThread_);

    Thread->AddRef();

    return Thread;
}
