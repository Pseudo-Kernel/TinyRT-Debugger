#pragma once

#include <Windows.h>
#include <Unknwn.h>
#include <OleAuto.h>
#include <msdbg.h>

#include <atomic>

class DebugEngine final :
    public IDebugEngine2
{
public:
    DebugEngine();

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

private:
    std::atomic<ULONG> ReferenceCount_ = 1;
};