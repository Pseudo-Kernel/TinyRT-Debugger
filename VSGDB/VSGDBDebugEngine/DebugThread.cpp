#include "DebugThread.h"

#include "DebugProgram.h"
#include "LogUtils.h"

DebugThread::DebugThread(
    DebugProgram* Program,
    DWORD ThreadId) :
    Program_(Program),
    ThreadId_(ThreadId)
{
    //
    // Non-owning.
    // DebugProgram owns DebugThread and outlives it.
    //
    VsgdbLogFormat(
        L"DebugThread created: ThreadId=%lu",
        ThreadId_);
}

DebugThread::~DebugThread()
{
    VsgdbLog(L"DebugThread destroyed");

    Program_ = nullptr;
}

HRESULT STDMETHODCALLTYPE
DebugThread::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugThread2))
    {
        *Object = static_cast<IDebugThread2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugThread::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
DebugThread::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
DebugThread::EnumFrameInfo(
    FRAMEINFO_FLAGS FieldSpec,
    UINT Radix,
    IEnumDebugFrameInfo2** Enum)
{
    VsgdbLogFormat(
        L"DebugThread::EnumFrameInfo: FieldSpec=0x%08x Radix=%u",
        FieldSpec,
        Radix);

    if (Enum == nullptr)
    {
        return E_POINTER;
    }

    *Enum = nullptr;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugThread::GetName(
    BSTR* Name)
{
    VsgdbLog(L"DebugThread::GetName");

    if (Name == nullptr)
    {
        return E_POINTER;
    }

    *Name = SysAllocString(L"CPU0");

    if (*Name == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugThread::SetThreadName(
    LPCOLESTR Name)
{
    VsgdbLogFormat(
        L"DebugThread::SetThreadName: Name=%s",
        Name != nullptr ? Name : L"(null)");

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugThread::GetProgram(
    IDebugProgram2** Program)
{
    VsgdbLog(L"DebugThread::GetProgram");

    if (Program == nullptr)
    {
        return E_POINTER;
    }

    *Program = static_cast<IDebugProgram2*>(Program_);

    if (*Program != nullptr)
    {
        (*Program)->AddRef();
        return S_OK;
    }

    return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE
DebugThread::CanSetNextStatement(
    IDebugStackFrame2* StackFrame,
    IDebugCodeContext2* CodeContext)
{
    VsgdbLogFormat(
        L"DebugThread::CanSetNextStatement: StackFrame=%p CodeContext=%p",
        StackFrame,
        CodeContext);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugThread::SetNextStatement(
    IDebugStackFrame2* StackFrame,
    IDebugCodeContext2* CodeContext)
{
    VsgdbLogFormat(
        L"DebugThread::SetNextStatement: StackFrame=%p CodeContext=%p",
        StackFrame,
        CodeContext);

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
DebugThread::GetThreadId(
    DWORD* ThreadId)
{
    VsgdbLog(L"DebugThread::GetThreadId");

    if (ThreadId == nullptr)
    {
        return E_POINTER;
    }

    *ThreadId = ThreadId_;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugThread::Suspend(
    DWORD* SuspendCount)
{
    VsgdbLog(L"DebugThread::Suspend");

    if (SuspendCount != nullptr)
    {
        *SuspendCount = 0;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugThread::Resume(
    DWORD* SuspendCount)
{
    VsgdbLog(L"DebugThread::Resume");

    if (SuspendCount != nullptr)
    {
        *SuspendCount = 0;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugThread::GetThreadProperties(
    THREADPROPERTY_FIELDS Fields,
    THREADPROPERTIES* Properties)
{
    VsgdbLogFormat(
        L"DebugThread::GetThreadProperties: Fields=0x%08x",
        Fields);

    if (Properties == nullptr)
    {
        return E_POINTER;
    }

    ZeroMemory(
        Properties,
        sizeof(*Properties));

    Properties->dwFields = Fields;

    if ((Fields & TPF_ID) != 0)
    {
        Properties->dwThreadId = ThreadId_;
    }

    if ((Fields & TPF_SUSPENDCOUNT) != 0)
    {
        Properties->dwSuspendCount = 0;
    }

    if ((Fields & TPF_STATE) != 0)
    {
        Properties->dwThreadState = THREADSTATE_RUNNING;
    }

    if ((Fields & TPF_PRIORITY) != 0)
    {
        Properties->bstrPriority = SysAllocString(L"Normal");
    }

    if ((Fields & TPF_NAME) != 0)
    {
        Properties->bstrName = SysAllocString(L"CPU0");
    }

    if ((Fields & TPF_LOCATION) != 0)
    {
        Properties->bstrLocation = SysAllocString(L"VSGDB synthetic CPU thread");
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugThread::GetLogicalThread(
    IDebugStackFrame2* StackFrame,
    IDebugLogicalThread2** LogicalThread)
{
    VsgdbLogFormat(
        L"DebugThread::GetLogicalThread: StackFrame=%p",
        StackFrame);

    if (LogicalThread == nullptr)
    {
        return E_POINTER;
    }

    *LogicalThread = nullptr;

    return E_NOTIMPL;
}