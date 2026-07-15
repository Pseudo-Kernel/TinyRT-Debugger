#pragma once

#include <atomic>
#include <msdbg.h>

class DebugEngineCreateEvent final :
    public IDebugEvent2,
    public IDebugEngineCreateEvent2
{
public:
    explicit DebugEngineCreateEvent(
        IDebugEngine2* Engine);

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    //
    // IDebugEvent2
    //
    HRESULT STDMETHODCALLTYPE GetAttributes(
        DWORD* EventAttributes) override;

    //
    // IDebugEngineCreateEvent2
    //
    HRESULT STDMETHODCALLTYPE GetEngine(
        IDebugEngine2** Engine) override;

private:
    ~DebugEngineCreateEvent();

    std::atomic<ULONG> ReferenceCount_ = 1;
    IDebugEngine2* Engine_ = nullptr;
};

class DebugProgramCreateEvent final :
    public IDebugEvent2,
    public IDebugProgramCreateEvent2
{
public:
    DebugProgramCreateEvent();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    //
    // IDebugEvent2
    //
    HRESULT STDMETHODCALLTYPE GetAttributes(
        DWORD* EventAttributes) override;

private:
    ~DebugProgramCreateEvent();

    std::atomic<ULONG> ReferenceCount_ = 1;
};
