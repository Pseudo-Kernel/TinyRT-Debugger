#pragma once

#include <Windows.h>
#include <Unknwn.h>
#include <OleAuto.h>
#include <msdbg.h>

#include <atomic>

class DebugThread;

class DebugProgram final :
    public IDebugProgram2
{
public:
    explicit DebugProgram(
        IDebugProcess2* Process);

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE EnumThreads(
        IEnumDebugThreads2** Enum) override;

    HRESULT STDMETHODCALLTYPE GetName(
        BSTR* Name) override;

    HRESULT STDMETHODCALLTYPE GetProcess(
        IDebugProcess2** Process) override;

    HRESULT STDMETHODCALLTYPE Terminate() override;

    HRESULT STDMETHODCALLTYPE Attach(
        IDebugEventCallback2* Callback) override;

    HRESULT STDMETHODCALLTYPE CanDetach() override;

    HRESULT STDMETHODCALLTYPE Detach() override;

    HRESULT STDMETHODCALLTYPE GetProgramId(
        GUID* ProgramId) override;

    HRESULT STDMETHODCALLTYPE GetDebugProperty(
        IDebugProperty2** Property) override;

    HRESULT STDMETHODCALLTYPE Execute() override;

    HRESULT STDMETHODCALLTYPE Continue(
        IDebugThread2* Thread) override;

    HRESULT STDMETHODCALLTYPE Step(
        IDebugThread2* Thread,
        STEPKIND StepKind,
        STEPUNIT StepUnit) override;

    HRESULT STDMETHODCALLTYPE CauseBreak() override;

    HRESULT STDMETHODCALLTYPE GetEngineInfo(
        BSTR* EngineName,
        GUID* EngineGuid) override;

    HRESULT STDMETHODCALLTYPE EnumCodeContexts(
        IDebugDocumentPosition2* DocumentPosition,
        IEnumDebugCodeContexts2** Enum) override;

    HRESULT STDMETHODCALLTYPE GetMemoryBytes(
        IDebugMemoryBytes2** MemoryBytes) override;

    HRESULT STDMETHODCALLTYPE GetDisassemblyStream(
        DISASSEMBLY_STREAM_SCOPE Scope,
        IDebugCodeContext2* CodeContext,
        IDebugDisassemblyStream2** DisassemblyStream) override;

    HRESULT STDMETHODCALLTYPE EnumModules(
        IEnumDebugModules2** Enum) override;

    HRESULT STDMETHODCALLTYPE GetENCUpdate(
        IDebugENCUpdate** Update) override;

    HRESULT STDMETHODCALLTYPE EnumCodePaths(
        LPCOLESTR Hint,
        IDebugCodeContext2* Start,
        IDebugStackFrame2* Frame,
        BOOL Source,
        IEnumCodePaths2** Enum,
        IDebugCodeContext2** Safety) override;

    HRESULT STDMETHODCALLTYPE WriteDump(
        DUMPTYPE DumpType,
        LPCOLESTR DumpUrl) override;

    IDebugThread2* GetMainThreadForEvent();

private:
    ~DebugProgram();

    std::atomic<ULONG> ReferenceCount_ = 1;
    GUID ProgramId_ = {};
    IDebugProcess2* Process_ = nullptr;
    DebugThread* MainThread_ = nullptr;
};
