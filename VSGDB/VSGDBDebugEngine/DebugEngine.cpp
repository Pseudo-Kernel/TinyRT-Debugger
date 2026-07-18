
#include "DebugEngine.h"
#include "DebugEvents.h"
#include "DebugProgram.h"
#include "DebugProgramNode.h"
#include "EnumDebugPrograms.h"

#include "LogUtils.h"
#include "VSGDBDebugEngineGuids.h"

#include <cwctype>
#include <string>




DebugEngine::DebugEngine()
    : ReferenceCount_(1)
{
    ProgramCreateContinuedEvent_ =
        CreateEventW(
            nullptr,
            TRUE,
            FALSE,
            nullptr);

    BreakContinuedEvent_ =
        CreateEventW(
            nullptr,
            TRUE,
            FALSE,
            nullptr);

    VsgdbLog(L"DebugEngine created");
}

DebugEngine::~DebugEngine()
{
    VsgdbLog(L"DebugEngine destroyed");

    if (VsgdbProgram_ != nullptr)
    {
        VsgdbProgram_->Release();
        VsgdbProgram_ = nullptr;
    }

    if (Program_ != nullptr)
    {
        Program_->Release();
        Program_ = nullptr;
    }

    if (Process_ != nullptr)
    {
        Process_->Release();
        Process_ = nullptr;
    }

    if (Callback_ != nullptr)
    {
        Callback_->Release();
        Callback_ = nullptr;
    }

    if (LaunchedThreadHandle_ != nullptr)
    {
        CloseHandle(LaunchedThreadHandle_);
        LaunchedThreadHandle_ = nullptr;
    }

    if (LaunchedProcessHandle_ != nullptr)
    {
        CloseHandle(LaunchedProcessHandle_);
        LaunchedProcessHandle_ = nullptr;
    }

    if (ProgramCreateContinuedEvent_ != nullptr)
    {
        CloseHandle(ProgramCreateContinuedEvent_);
        ProgramCreateContinuedEvent_ = nullptr;
    }

    if (BreakContinuedEvent_ != nullptr)
    {
        CloseHandle(BreakContinuedEvent_);
        BreakContinuedEvent_ = nullptr;
    }
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

    std::vector<IDebugProgram2*> ProgramList;

    if (VsgdbProgram_ != nullptr)
    {
        ProgramList.push_back(
            static_cast<IDebugProgram2*>(VsgdbProgram_));
    }

    EnumDebugPrograms* Enum =
        new (std::nothrow) EnumDebugPrograms(ProgramList);

    if (Enum == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    *Programs = static_cast<IEnumDebugPrograms2*>(Enum);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::Attach(
    IDebugProgram2** Programs,
    IDebugProgramNode2** ProgramNodes,
    DWORD ProgramCount,
    IDebugEventCallback2* Callback,
    ATTACH_REASON Reason)
{
    VsgdbLogFormat(
        L"DebugEngine::Attach: ProgramCount=%lu Callback=%p Reason=%lu",
        ProgramCount,
        Callback,
        Reason);

    return S_OK;
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
    VsgdbLogFormat(
        L"DebugEngine::DestroyProgram: Program=%p VsgdbProgram_=%p",
        Program,
        VsgdbProgram_);

    if (Program == nullptr)
    {
        return E_POINTER;
    }

    if (VsgdbProgram_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::DestroyProgram: no VSGDB program");
        return S_OK;
    }

    IDebugProgram2* OwnedProgram =
        static_cast<IDebugProgram2*>(VsgdbProgram_);

    if (Program != OwnedProgram)
    {
        VsgdbLog(L"DebugEngine::DestroyProgram: ignoring non-VSGDB program");
        return S_FALSE;
    }

    //
    // The SDM is telling us that the VSGDB program is no longer needed.
    // Drop the engine's ownership. Any outstanding COM references held by VS
    // or event objects will keep the program alive until they are released.
    //
    VsgdbProgram_->Release();
    VsgdbProgram_ = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::ContinueFromSynchronousEvent(
    IDebugEvent2* Event)
{
    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: Event=%p",
        Event);

    if (Event == nullptr)
    {
        return S_OK;
    }

    IDebugEngineCreateEvent2* EngineCreate = nullptr;
    HRESULT Hr =
        Event->QueryInterface(
            __uuidof(IDebugEngineCreateEvent2),
            reinterpret_cast<void**>(&EngineCreate));

    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: QI EngineCreate Hr=0x%08x Ptr=%p",
        Hr,
        EngineCreate);

    if (EngineCreate != nullptr)
    {
        EngineCreate->Release();
        return S_OK;
    }

    IDebugProgramCreateEvent2* ProgramCreate = nullptr;
    Hr =
        Event->QueryInterface(
            __uuidof(IDebugProgramCreateEvent2),
            reinterpret_cast<void**>(&ProgramCreate));

    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: QI ProgramCreate Hr=0x%08x Ptr=%p",
        Hr,
        ProgramCreate);

    if (ProgramCreate != nullptr)
    {
        ProgramCreate->Release();

        if (ProgramCreateContinuedEvent_ != nullptr)
        {
            SetEvent(ProgramCreateContinuedEvent_);
        }

        return S_OK;
    }

    IDebugThreadCreateEvent2* ThreadCreate = nullptr;
    Hr =
        Event->QueryInterface(
            __uuidof(IDebugThreadCreateEvent2),
            reinterpret_cast<void**>(&ThreadCreate));

    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: QI ThreadCreate Hr=0x%08x Ptr=%p",
        Hr,
        ThreadCreate);

    if (ThreadCreate != nullptr)
    {
        ThreadCreate->Release();
        return S_OK;
    }

    IDebugLoadCompleteEvent2* LoadComplete = nullptr;
    Hr =
        Event->QueryInterface(
            __uuidof(IDebugLoadCompleteEvent2),
            reinterpret_cast<void**>(&LoadComplete));

    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: QI LoadComplete Hr=0x%08x Ptr=%p",
        Hr,
        LoadComplete);

    if (LoadComplete != nullptr)
    {
        LoadComplete->Release();
        return S_OK;
    }

    IDebugEntryPointEvent2* EntryPoint = nullptr;
    Hr =
        Event->QueryInterface(
            __uuidof(IDebugEntryPointEvent2),
            reinterpret_cast<void**>(&EntryPoint));

    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: QI EntryPoint Hr=0x%08x Ptr=%p",
        Hr,
        EntryPoint);

    if (EntryPoint != nullptr)
    {
        EntryPoint->Release();
        return S_OK;
    }

    IDebugBreakEvent2* Break = nullptr;
    Hr =
        Event->QueryInterface(
            __uuidof(IDebugBreakEvent2),
            reinterpret_cast<void**>(&Break));

    VsgdbLogFormat(
        L"DebugEngine::ContinueFromSynchronousEvent: QI Break Hr=0x%08x Ptr=%p",
        Hr,
        Break);

    if (Break != nullptr)
    {
        Break->Release();

        if (BreakContinuedEvent_ != nullptr)
        {
            SetEvent(BreakContinuedEvent_);
        }

        return S_OK;
    }

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
    UNREFERENCED_PARAMETER(Server);
    UNREFERENCED_PARAMETER(Env);
    UNREFERENCED_PARAMETER(Options);
    UNREFERENCED_PARAMETER(LaunchFlags);
    UNREFERENCED_PARAMETER(Callback);

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: Exe=%s Args=%s Dir=%s Options=%s",
        Exe != nullptr ? Exe : L"(null)",
        Args != nullptr ? Args : L"(null)",
        Dir != nullptr ? Dir : L"(null)",
        Options != nullptr ? Options : L"(null)");

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: Callback=%p",
        Callback);

    if (Port == nullptr || Process == nullptr || Exe == nullptr)
    {
        return E_POINTER;
    }

    *Process = nullptr;

    IDebugDefaultPort2* DefaultPort = nullptr;

    HRESULT Hr =
        Port->QueryInterface(
            __uuidof(IDebugDefaultPort2),
            reinterpret_cast<void**>(&DefaultPort));

    if (FAILED(Hr) || DefaultPort == nullptr)
    {
        VsgdbLogFormat(
            L"DebugEngine::LaunchSuspended: QI IDebugDefaultPort2 failed Hr=0x%08x",
            Hr);

        return Hr;
    }

    std::wstring CommandLine;

    CommandLine += L"\"";
    CommandLine += Exe;
    CommandLine += L"\"";

    if (Args != nullptr && Args[0] != L'\0')
    {
        CommandLine += L" ";
        CommandLine += Args;
    }

    STARTUPINFOW StartupInfo = {};
    StartupInfo.cb = sizeof(StartupInfo);

    if (StandardInput != 0 ||
        StandardOutput != 0 ||
        StandardError != 0)
    {
        StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
        StartupInfo.hStdInput =
            reinterpret_cast<HANDLE>(
                static_cast<uintptr_t>(StandardInput));
        StartupInfo.hStdOutput =
            reinterpret_cast<HANDLE>(
                static_cast<uintptr_t>(StandardOutput));
        StartupInfo.hStdError =
            reinterpret_cast<HANDLE>(
                static_cast<uintptr_t>(StandardError));
    }

    PROCESS_INFORMATION ProcessInformation = {};

    BOOL Created =
        CreateProcessW(
            Exe,
            CommandLine.data(),
            nullptr,
            nullptr,
            TRUE,
            CREATE_SUSPENDED,
            nullptr,
            Dir,
            &StartupInfo,
            &ProcessInformation);

    if (!Created)
    {
        DWORD Error = GetLastError();

        VsgdbLogFormat(
            L"DebugEngine::LaunchSuspended: CreateProcessW failed Error=%lu",
            Error);

        DefaultPort->Release();

        return HRESULT_FROM_WIN32(Error);
    }

    LaunchedProcessId_ = ProcessInformation.dwProcessId;
    LaunchedProcessHandle_ = ProcessInformation.hProcess;
    LaunchedThreadHandle_ = ProcessInformation.hThread;

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: Created PID=%lu TID=%lu",
        ProcessInformation.dwProcessId,
        ProcessInformation.dwThreadId);

    AD_PROCESS_ID ProcessId = {};
    ProcessId.ProcessIdType = AD_PROCESS_ID_SYSTEM;
    ProcessId.ProcessId.dwProcessId = ProcessInformation.dwProcessId;

    Hr = DefaultPort->GetProcess(
        ProcessId,
        Process);

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: DefaultPort->GetProcess Hr=0x%08x Process=%p",
        Hr,
        Process != nullptr ? *Process : nullptr);

    DefaultPort->Release();

    if (FAILED(Hr))
    {
        ::TerminateProcess(
            LaunchedProcessHandle_,
            1);

        CloseHandle(LaunchedThreadHandle_);
        CloseHandle(LaunchedProcessHandle_);

        LaunchedThreadHandle_ = nullptr;
        LaunchedProcessHandle_ = nullptr;
        LaunchedProcessId_ = 0;

        return Hr;
    }

    if (Callback != nullptr)
    {
        Callback->AddRef();

        if (Callback_ != nullptr)
        {
            Callback_->Release();
        }

        Callback_ = Callback;
    }

    if (*Process != nullptr)
    {
        (*Process)->AddRef();

        if (Process_ != nullptr)
        {
            Process_->Release();
        }

        Process_ = *Process;
    }

#if 1
    if (VsgdbProgram_ != nullptr)
    {
        VsgdbProgram_->Release();
        VsgdbProgram_ = nullptr;
    }

    VsgdbProgram_ =
        new (std::nothrow) DebugProgram(Process_);

    if (VsgdbProgram_ == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: Created VsgdbProgram_=%p",
        VsgdbProgram_);
#endif

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: Stored Callback_=%p Process_=%p",
        Callback_,
        Process_);

    //
    // Diagnostic only: this returns the default port's "Native" program,
    // not a VSGDB-owned program.
    //
    HRESULT ProgramHr =
        CaptureProgramFromProcess(Process_);

    VsgdbLogFormat(
        L"DebugEngine::LaunchSuspended: CaptureProgramFromProcess Hr=0x%08x Program_=%p",
        ProgramHr,
        Program_);

    if (Callback_ == nullptr)
    {
        VsgdbLog(
            L"DebugEngine::LaunchSuspended: Callback is null, skipping EngineCreateEvent");
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngine::ResumeProcess(
    IDebugProcess2* Process)
{
    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: Process=%p",
        Process);

    HRESULT Hr =
        AddProgramNodeToPortNotify(Process);

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: AddProgramNodeToPortNotify Hr=0x%08x",
        Hr);

    if (ProgramCreateContinuedEvent_ != nullptr)
    {
        ResetEvent(ProgramCreateContinuedEvent_);
    }

    HRESULT EngineCreateHr =
        SendEngineCreateEvent();

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: EngineCreateEvent Hr=0x%08x",
        EngineCreateHr);

    if (FAILED(EngineCreateHr))
    {
        return EngineCreateHr;
    }

    HRESULT ProgramCreateHr =
        SendProgramCreateEvent();

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: ProgramCreateEvent Hr=0x%08x",
        ProgramCreateHr);

    if (FAILED(ProgramCreateHr))
    {
        return ProgramCreateHr;
    }

    HRESULT ThreadCreateHr =
        SendThreadCreateEvent();

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: ThreadCreateEvent Hr=0x%08x",
        ThreadCreateHr);

    if (FAILED(ThreadCreateHr))
    {
        return ThreadCreateHr;
    }

    HRESULT LoadCompleteHr =
        SendLoadCompleteEvent();

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: LoadCompleteEvent Hr=0x%08x",
        LoadCompleteHr);

    if (FAILED(LoadCompleteHr))
    {
        return LoadCompleteHr;
    }

    HRESULT EntryPointHr =
        SendEntryPointEvent();

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: EntryPointEvent Hr=0x%08x",
        EntryPointHr);

    if (FAILED(EntryPointHr))
    {
        return EntryPointHr;
    }

    if (ProgramCreateContinuedEvent_ != nullptr)
    {
        DWORD WaitResult =
            WaitForSingleObject(
                ProgramCreateContinuedEvent_,
                5000);

        VsgdbLogFormat(
            L"DebugEngine::ResumeProcess: ProgramCreate wait result=0x%08x",
            WaitResult);
    }


    HRESULT BreakHr =
        SendBreakEvent();

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: BreakEvent Hr=0x%08x",
        BreakHr);

    if (FAILED(BreakHr))
    {
        return BreakHr;
    }


    //
    // Finally resume the Win32 anchor process.
    //

    DWORD Result =
        ResumeThread(
            LaunchedThreadHandle_);

    if (Result == static_cast<DWORD>(-1))
    {
        DWORD Error = GetLastError();

        VsgdbLogFormat(
            L"DebugEngine::ResumeProcess: ResumeThread failed Error=%lu",
            Error);

        return HRESULT_FROM_WIN32(Error);
    }

    VsgdbLogFormat(
        L"DebugEngine::ResumeProcess: ResumeThread previous suspend count=%lu",
        Result);

    CloseHandle(LaunchedThreadHandle_);
    LaunchedThreadHandle_ = nullptr;

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
    IDebugProcess2* Process)
{
    VsgdbLogFormat(
        L"DebugEngine::TerminateProcess: Process=%p",
        Process);

    if (LaunchedProcessHandle_ != nullptr)
    {
        ::TerminateProcess(
            LaunchedProcessHandle_,
            1);

        CloseHandle(LaunchedProcessHandle_);
        LaunchedProcessHandle_ = nullptr;
    }

    if (LaunchedThreadHandle_ != nullptr)
    {
        CloseHandle(LaunchedThreadHandle_);
        LaunchedThreadHandle_ = nullptr;
    }

    LaunchedProcessId_ = 0;

    return S_OK;
}



// 
// Helpers.
// 

HRESULT
DebugEngine::CaptureProgramFromProcess(
    IDebugProcess2* Process)
{
    VsgdbLogFormat(
        L"DebugEngine::CaptureProgramFromProcess: Process=%p",
        Process);

    if (Process == nullptr)
    {
        return E_POINTER;
    }

    IEnumDebugPrograms2* EnumPrograms = nullptr;

    HRESULT Hr =
        Process->EnumPrograms(
            &EnumPrograms);

    VsgdbLogFormat(
        L"DebugEngine::CaptureProgramFromProcess: EnumPrograms Hr=0x%08x Enum=%p",
        Hr,
        EnumPrograms);

    if (FAILED(Hr) || EnumPrograms == nullptr)
    {
        return Hr;
    }

    ULONG Fetched = 0;
    IDebugProgram2* Program = nullptr;

    Hr =
        EnumPrograms->Next(
            1,
            &Program,
            &Fetched);

    VsgdbLogFormat(
        L"DebugEngine::CaptureProgramFromProcess: Next Hr=0x%08x Fetched=%lu Program=%p",
        Hr,
        Fetched,
        Program);

    EnumPrograms->Release();

    if (FAILED(Hr))
    {
        return Hr;
    }

    if (Fetched != 1 || Program == nullptr)
    {
        return E_FAIL;
    }

    LogProgramInfo(Program);

    if (Program_ != nullptr)
    {
        Program_->Release();
        Program_ = nullptr;
    }

    Program_ = Program;
    Program_->AddRef();

    //
    // Release the local enumeration reference. Program_ keeps its own reference.
    //
    Program->Release();

    VsgdbLogFormat(
        L"DebugEngine::CaptureProgramFromProcess: Stored Program_=%p",
        Program_);

    return S_OK;
}

void
DebugEngine::LogProgramInfo(
    IDebugProgram2* Program)
{
    if (Program == nullptr)
    {
        VsgdbLog(L"DebugEngine::LogProgramInfo: Program is null");
        return;
    }

    BSTR Name = nullptr;

    HRESULT Hr =
        Program->GetName(
            &Name);

    if (SUCCEEDED(Hr) && Name != nullptr)
    {
        VsgdbLogFormat(
            L"DebugEngine::LogProgramInfo: Name=%s",
            Name);

        SysFreeString(Name);
    }
    else
    {
        VsgdbLogFormat(
            L"DebugEngine::LogProgramInfo: GetName Hr=0x%08x",
            Hr);
    }

    GUID ProgramId = {};

    Hr =
        Program->GetProgramId(
            &ProgramId);

    VsgdbLogFormat(
        L"DebugEngine::LogProgramInfo: GetProgramId Hr=0x%08x",
        Hr);
}

HRESULT
DebugEngine::SendEvent(
    IDebugEvent2* Event,
    REFIID EventInterfaceId,
    DWORD Attributes,
    IDebugProgram2* Program,
    IDebugThread2* Thread)
{
    VsgdbLogFormat(
        L"DebugEngine::SendEvent: Callback_=%p Process_=%p Program=%p Thread=%p Event=%p Attributes=0x%08x",
        Callback_,
        Process_,
        Program,
        Thread,
        Event,
        Attributes);

    if (Callback_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendEvent: Callback_ is null");
        return E_UNEXPECTED;
    }

    if (Event == nullptr)
    {
        return E_POINTER;
    }

    HRESULT Hr =
        Callback_->Event(
            static_cast<IDebugEngine2*>(this),
            Process_,
            Program,
            Thread,
            Event,
            EventInterfaceId,
            Attributes);

    VsgdbLogFormat(
        L"DebugEngine::SendEvent: Callback_->Event Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::SendEngineCreateEvent()
{
    DebugEngineCreateEvent* Event =
        new (std::nothrow) DebugEngineCreateEvent(
            static_cast<IDebugEngine2*>(this));

    if (Event == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT Hr =
        SendEvent(
            static_cast<IDebugEvent2*>(Event),
            __uuidof(IDebugEngineCreateEvent2),
            EVENT_SYNCHRONOUS,
            nullptr,
            nullptr);

    Event->Release();

    VsgdbLogFormat(
        L"DebugEngine::SendEngineCreateEvent: Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::SendProgramCreateEvent()
{
    if (VsgdbProgram_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendProgramCreateEvent: VsgdbProgram_ is null");
        return E_UNEXPECTED;
    }

    DebugProgramCreateEvent* Event =
        new (std::nothrow) DebugProgramCreateEvent();

    if (Event == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT Hr =
        SendEvent(
            static_cast<IDebugEvent2*>(Event),
            __uuidof(IDebugProgramCreateEvent2),
            EVENT_SYNCHRONOUS,
            static_cast<IDebugProgram2*>(VsgdbProgram_),
            nullptr);

    Event->Release();

    VsgdbLogFormat(
        L"DebugEngine::SendProgramCreateEvent: Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::SendThreadCreateEvent()
{
    if (VsgdbProgram_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendThreadCreateEvent: VsgdbProgram_ is null");
        return E_UNEXPECTED;
    }

    IDebugThread2* Thread =
        VsgdbProgram_->GetMainThreadForEvent();

    if (Thread == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendThreadCreateEvent: no main thread");
        return E_UNEXPECTED;
    }

    DebugThreadCreateEvent* Event =
        new (std::nothrow) DebugThreadCreateEvent();

    if (Event == nullptr)
    {
        Thread->Release();
        return E_OUTOFMEMORY;
    }

    HRESULT Hr =
        SendEvent(
            static_cast<IDebugEvent2*>(Event),
            __uuidof(IDebugThreadCreateEvent2),
            EVENT_SYNCHRONOUS,
            static_cast<IDebugProgram2*>(VsgdbProgram_),
            Thread);

    Event->Release();
    Thread->Release();

    VsgdbLogFormat(
        L"DebugEngine::SendThreadCreateEvent: Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::SendLoadCompleteEvent()
{
    if (VsgdbProgram_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendLoadCompleteEvent: VsgdbProgram_ is null");
        return E_UNEXPECTED;
    }

    IDebugThread2* Thread =
        VsgdbProgram_->GetMainThreadForEvent();

    if (Thread == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendLoadCompleteEvent: no main thread");
        return E_UNEXPECTED;
    }

    DebugLoadCompleteEvent* Event =
        new (std::nothrow) DebugLoadCompleteEvent();

    if (Event == nullptr)
    {
        Thread->Release();
        return E_OUTOFMEMORY;
    }

    HRESULT Hr =
        SendEvent(
            static_cast<IDebugEvent2*>(Event),
            __uuidof(IDebugLoadCompleteEvent2),
            EVENT_SYNCHRONOUS,
            static_cast<IDebugProgram2*>(VsgdbProgram_),
            Thread);

    Event->Release();
    Thread->Release();

    VsgdbLogFormat(
        L"DebugEngine::SendLoadCompleteEvent: Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::SendBreakEvent()
{
    if (VsgdbProgram_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendBreakEvent: VsgdbProgram_ is null");
        return E_UNEXPECTED;
    }

    IDebugThread2* Thread =
        VsgdbProgram_->GetMainThreadForEvent();

    if (Thread == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendBreakEvent: no main thread");
        return E_UNEXPECTED;
    }

    DebugBreakEvent* Event =
        new (std::nothrow) DebugBreakEvent();

    if (Event == nullptr)
    {
        Thread->Release();
        return E_OUTOFMEMORY;
    }

    HRESULT Hr =
        SendEvent(
            static_cast<IDebugEvent2*>(Event),
            __uuidof(IDebugBreakEvent2),
            EVENT_SYNC_STOP,
            static_cast<IDebugProgram2*>(VsgdbProgram_),
            Thread);

    Event->Release();
    Thread->Release();

    VsgdbLogFormat(
        L"DebugEngine::SendBreakEvent: Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::SendEntryPointEvent()
{
    if (VsgdbProgram_ == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendEntryPointEvent: VsgdbProgram_ is null");
        return E_UNEXPECTED;
    }

    IDebugThread2* Thread =
        VsgdbProgram_->GetMainThreadForEvent();

    if (Thread == nullptr)
    {
        VsgdbLog(L"DebugEngine::SendEntryPointEvent: no main thread");
        return E_UNEXPECTED;
    }

    DebugEntryPointEvent* Event =
        new (std::nothrow) DebugEntryPointEvent();

    if (Event == nullptr)
    {
        Thread->Release();
        return E_OUTOFMEMORY;
    }

    HRESULT Hr =
        SendEvent(
            static_cast<IDebugEvent2*>(Event),
            __uuidof(IDebugEntryPointEvent2),
            EVENT_SYNCHRONOUS,
            static_cast<IDebugProgram2*>(VsgdbProgram_),
            Thread);

    Event->Release();
    Thread->Release();

    VsgdbLogFormat(
        L"DebugEngine::SendEntryPointEvent: Hr=0x%08x",
        Hr);

    return Hr;
}

HRESULT
DebugEngine::AddProgramNodeToPortNotify(
    IDebugProcess2* Process)
{
    if (Process == nullptr)
    {
        return E_POINTER;
    }

    IDebugPort2* Port = nullptr;

    HRESULT Hr =
        Process->GetPort(
            &Port);

    VsgdbLogFormat(
        L"DebugEngine::AddProgramNodeToPortNotify: GetPort Hr=0x%08x Port=%p",
        Hr,
        Port);

    if (FAILED(Hr) || Port == nullptr)
    {
        return Hr;
    }

    IDebugDefaultPort2* DefaultPort = nullptr;

    Hr =
        Port->QueryInterface(
            __uuidof(IDebugDefaultPort2),
            reinterpret_cast<void**>(&DefaultPort));

    VsgdbLogFormat(
        L"DebugEngine::AddProgramNodeToPortNotify: QI IDebugDefaultPort2 Hr=0x%08x Ptr=%p",
        Hr,
        DefaultPort);

    Port->Release();

    if (FAILED(Hr) || DefaultPort == nullptr)
    {
        return Hr;
    }

    IDebugPortNotify2* PortNotify = nullptr;

    Hr =
        DefaultPort->GetPortNotify(
            &PortNotify);

    VsgdbLogFormat(
        L"DebugEngine::AddProgramNodeToPortNotify: GetPortNotify Hr=0x%08x Ptr=%p",
        Hr,
        PortNotify);

    DefaultPort->Release();

    if (FAILED(Hr) || PortNotify == nullptr)
    {
        return Hr;
    }

    DebugProgramNode* Node =
        new (std::nothrow) DebugProgramNode();

    if (Node == nullptr)
    {
        PortNotify->Release();
        return E_OUTOFMEMORY;
    }

    Hr =
        PortNotify->AddProgramNode(
            static_cast<IDebugProgramNode2*>(Node));

    VsgdbLogFormat(
        L"DebugEngine::AddProgramNodeToPortNotify: AddProgramNode Hr=0x%08x",
        Hr);

    Node->Release();
    PortNotify->Release();

    return Hr;
}