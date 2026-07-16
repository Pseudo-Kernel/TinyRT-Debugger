#include "EnumDebugThreads.h"

#include "LogUtils.h"

EnumDebugThreads::EnumDebugThreads(
    const std::vector<IDebugThread2*>& Threads)
{
    Threads_.reserve(
        Threads.size());

    for (IDebugThread2* Thread : Threads)
    {
        if (Thread != nullptr)
        {
            Thread->AddRef();
            Threads_.push_back(Thread);
        }
    }

    VsgdbLogFormat(
        L"EnumDebugThreads created: Count=%llu",
        static_cast<unsigned long long>(Threads_.size()));
}

EnumDebugThreads::~EnumDebugThreads()
{
    VsgdbLog(L"EnumDebugThreads destroyed");

    for (IDebugThread2* Thread : Threads_)
    {
        if (Thread != nullptr)
        {
            Thread->Release();
        }
    }

    Threads_.clear();
}

HRESULT STDMETHODCALLTYPE
EnumDebugThreads::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IEnumDebugThreads2))
    {
        *Object = static_cast<IEnumDebugThreads2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
EnumDebugThreads::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
EnumDebugThreads::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
EnumDebugThreads::Next(
    ULONG Count,
    IDebugThread2** Threads,
    ULONG* Fetched)
{
    VsgdbLogFormat(
        L"EnumDebugThreads::Next: Count=%lu",
        Count);

    if (Threads == nullptr)
    {
        return E_POINTER;
    }

    ULONG Actual = 0;

    while (Actual < Count && Index_ < Threads_.size())
    {
        IDebugThread2* Thread =
            Threads_[Index_++];

        Thread->AddRef();
        Threads[Actual++] = Thread;
    }

    if (Fetched != nullptr)
    {
        *Fetched = Actual;
    }

    return Actual == Count ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE
EnumDebugThreads::Skip(
    ULONG Count)
{
    VsgdbLogFormat(
        L"EnumDebugThreads::Skip: Count=%lu",
        Count);

    const ULONG Remaining =
        static_cast<ULONG>(Threads_.size() - Index_);

    Index_ += min(Count, Remaining);

    return Count <= Remaining ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE
EnumDebugThreads::Reset()
{
    VsgdbLog(L"EnumDebugThreads::Reset");

    Index_ = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
EnumDebugThreads::Clone(
    IEnumDebugThreads2** Enum)
{
    VsgdbLog(L"EnumDebugThreads::Clone");

    if (Enum == nullptr)
    {
        return E_POINTER;
    }

    EnumDebugThreads* CloneEnum =
        new (std::nothrow) EnumDebugThreads(Threads_);

    if (CloneEnum == nullptr)
    {
        *Enum = nullptr;
        return E_OUTOFMEMORY;
    }

    CloneEnum->Index_ = Index_;
    *Enum = CloneEnum;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
EnumDebugThreads::GetCount(
    ULONG* Count)
{
    VsgdbLog(L"EnumDebugThreads::GetCount");

    if (Count == nullptr)
    {
        return E_POINTER;
    }

    *Count = static_cast<ULONG>(Threads_.size());

    return S_OK;
}