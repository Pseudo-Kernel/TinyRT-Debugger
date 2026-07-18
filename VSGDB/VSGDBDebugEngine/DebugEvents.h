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

class DebugThreadCreateEvent final :
    public IDebugEvent2,
    public IDebugThreadCreateEvent2
{
public:
    DebugThreadCreateEvent();

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
    ~DebugThreadCreateEvent();

    std::atomic<ULONG> ReferenceCount_ = 1;
};

class DebugLoadCompleteEvent final :
    public IDebugEvent2,
    public IDebugLoadCompleteEvent2
{
public:
    DebugLoadCompleteEvent();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetAttributes(
        DWORD* EventAttributes) override;

private:
    ~DebugLoadCompleteEvent();

    std::atomic<ULONG> ReferenceCount_ = 1;
};

class DebugBreakEvent final :
    public IDebugEvent2,
    public IDebugBreakEvent2
{
public:
    DebugBreakEvent();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetAttributes(
        DWORD* EventAttributes) override;

private:
    ~DebugBreakEvent();

    std::atomic<ULONG> ReferenceCount_ = 1;
};

class DebugEntryPointEvent final :
    public IDebugEvent2,
    public IDebugEntryPointEvent2
{
public:
    DebugEntryPointEvent();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetAttributes(
        DWORD* EventAttributes) override;

private:
    ~DebugEntryPointEvent();

    std::atomic<ULONG> ReferenceCount_ = 1;
};
