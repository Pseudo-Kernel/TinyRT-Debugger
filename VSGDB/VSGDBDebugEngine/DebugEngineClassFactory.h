#pragma once

#include <Windows.h>
#include <Unknwn.h>

#include <atomic>

class DebugEngineClassFactory final :
    public IClassFactory
{
public:
    DebugEngineClassFactory();

    //
    // IUnknown
    //
    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    //
    // IClassFactory
    //
    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown* Outer,
        REFIID InterfaceId,
        void** Object) override;

    HRESULT STDMETHODCALLTYPE LockServer(
        BOOL Lock) override;

private:
    std::atomic<ULONG> ReferenceCount;
};
