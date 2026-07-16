#pragma once

#include <Windows.h>
#include <Unknwn.h>
#include <OleAuto.h>
#include <msdbg.h>

#include <atomic>

class DebugProgram;

class DebugThread final :
    public IDebugThread2
{
public:
    explicit DebugThread(
        DebugProgram* Program,
        DWORD ThreadId);

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE EnumFrameInfo(
        FRAMEINFO_FLAGS FieldSpec,
        UINT Radix,
        IEnumDebugFrameInfo2** Enum) override;

    HRESULT STDMETHODCALLTYPE GetName(
        BSTR* Name) override;

    HRESULT STDMETHODCALLTYPE SetThreadName(
        LPCOLESTR Name) override;

    HRESULT STDMETHODCALLTYPE GetProgram(
        IDebugProgram2** Program) override;

    HRESULT STDMETHODCALLTYPE CanSetNextStatement(
        IDebugStackFrame2* StackFrame,
        IDebugCodeContext2* CodeContext) override;

    HRESULT STDMETHODCALLTYPE SetNextStatement(
        IDebugStackFrame2* StackFrame,
        IDebugCodeContext2* CodeContext) override;

    HRESULT STDMETHODCALLTYPE GetThreadId(
        DWORD* ThreadId) override;

    HRESULT STDMETHODCALLTYPE Suspend(
        DWORD* SuspendCount) override;

    HRESULT STDMETHODCALLTYPE Resume(
        DWORD* SuspendCount) override;

    HRESULT STDMETHODCALLTYPE GetThreadProperties(
        THREADPROPERTY_FIELDS Fields,
        THREADPROPERTIES* Properties) override;

    HRESULT STDMETHODCALLTYPE GetLogicalThread(
        IDebugStackFrame2* StackFrame,
        IDebugLogicalThread2** LogicalThread) override;

private:
    ~DebugThread();

    std::atomic<ULONG> ReferenceCount_ = 1;
    DWORD ThreadId_ = 0;

    //
    // Non-owning. DebugProgram owns DebugThread and outlives it.
    //
    DebugProgram* Program_ = nullptr;
};