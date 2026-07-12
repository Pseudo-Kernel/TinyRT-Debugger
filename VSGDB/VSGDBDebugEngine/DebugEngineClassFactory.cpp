#include "DebugEngineClassFactory.h"

#include "DebugEngine.h"
#include "LogUtils.h"

#include <new>

extern std::atomic<ULONG> g_DllObjectCount;
extern std::atomic<ULONG> g_DllLockCount;

DebugEngineClassFactory::DebugEngineClassFactory()
    : ReferenceCount(1)
{
    g_DllObjectCount.fetch_add(1, std::memory_order_relaxed);
    VsgdbLog(L"DebugEngineClassFactory created");
}

HRESULT STDMETHODCALLTYPE
DebugEngineClassFactory::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IClassFactory))
    {
        *Object = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugEngineClassFactory::AddRef()
{
    return ReferenceCount.fetch_add(
        1,
        std::memory_order_relaxed) + 1;
}

ULONG STDMETHODCALLTYPE
DebugEngineClassFactory::Release()
{
    ULONG NewReferenceCount =
        ReferenceCount.fetch_sub(
            1,
            std::memory_order_acq_rel) - 1;

    if (NewReferenceCount == 0)
    {
        VsgdbLog(L"DebugEngineClassFactory destroyed");

        g_DllObjectCount.fetch_sub(
            1,
            std::memory_order_relaxed);

        delete this;
    }

    return NewReferenceCount;
}

HRESULT STDMETHODCALLTYPE
DebugEngineClassFactory::CreateInstance(
    IUnknown* Outer,
    REFIID InterfaceId,
    void** Object)
{
    VsgdbLog(L"DebugEngineClassFactory::CreateInstance");

    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (Outer != nullptr)
    {
        return CLASS_E_NOAGGREGATION;
    }

    DebugEngine* Engine =
        new (std::nothrow) DebugEngine();

    if (Engine == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT Result =
        Engine->QueryInterface(
            InterfaceId,
            Object);

    Engine->Release();

    return Result;
}

HRESULT STDMETHODCALLTYPE
DebugEngineClassFactory::LockServer(
    BOOL Lock)
{
    if (Lock)
    {
        g_DllLockCount.fetch_add(
            1,
            std::memory_order_relaxed);
    }
    else
    {
        g_DllLockCount.fetch_sub(
            1,
            std::memory_order_relaxed);
    }

    return S_OK;
}