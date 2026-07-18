#include "EnumDebugPrograms.h"
#include "LogUtils.h"

EnumDebugPrograms::EnumDebugPrograms(
    const std::vector<IDebugProgram2*>& Programs)
{
    for (IDebugProgram2* Program : Programs)
    {
        if (Program != nullptr)
        {
            Program->AddRef();
            Programs_.push_back(Program);
        }
    }

    VsgdbLogFormat(
        L"EnumDebugPrograms created: Count=%llu",
        static_cast<unsigned long long>(Programs_.size()));
}

EnumDebugPrograms::~EnumDebugPrograms()
{
    VsgdbLog(L"EnumDebugPrograms destroyed");

    for (IDebugProgram2* Program : Programs_)
    {
        if (Program != nullptr)
        {
            Program->Release();
        }
    }

    Programs_.clear();
}

HRESULT STDMETHODCALLTYPE
EnumDebugPrograms::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IEnumDebugPrograms2))
    {
        *Object = static_cast<IEnumDebugPrograms2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
EnumDebugPrograms::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
EnumDebugPrograms::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
EnumDebugPrograms::Next(
    ULONG Count,
    IDebugProgram2** Programs,
    ULONG* Fetched)
{
    VsgdbLogFormat(
        L"EnumDebugPrograms::Next: Count=%lu",
        Count);

    if (Programs == nullptr)
    {
        return E_POINTER;
    }

    if (Fetched != nullptr)
    {
        *Fetched = 0;
    }

    ULONG Actual = 0;

    while (Actual < Count && Index_ < Programs_.size())
    {
        IDebugProgram2* Program = Programs_[Index_];

        Programs[Actual] = Program;

        if (Program != nullptr)
        {
            Program->AddRef();
        }

        ++Actual;
        ++Index_;
    }

    if (Fetched != nullptr)
    {
        *Fetched = Actual;
    }

    return (Actual == Count) ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE
EnumDebugPrograms::Skip(
    ULONG Count)
{
    VsgdbLogFormat(
        L"EnumDebugPrograms::Skip: Count=%lu",
        Count);

    const ULONG Remaining =
        static_cast<ULONG>(Programs_.size() - Index_);

    const ULONG Advance =
        (Count < Remaining) ? Count : Remaining;

    Index_ += Advance;

    return (Advance == Count) ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE
EnumDebugPrograms::Reset()
{
    VsgdbLog(L"EnumDebugPrograms::Reset");

    Index_ = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
EnumDebugPrograms::Clone(
    IEnumDebugPrograms2** Enum)
{
    VsgdbLog(L"EnumDebugPrograms::Clone");

    if (Enum == nullptr)
    {
        return E_POINTER;
    }

    *Enum = nullptr;

    EnumDebugPrograms* NewEnum =
        new (std::nothrow) EnumDebugPrograms(Programs_);

    if (NewEnum == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    NewEnum->Index_ = Index_;

    *Enum = static_cast<IEnumDebugPrograms2*>(NewEnum);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
EnumDebugPrograms::GetCount(
    ULONG* Count)
{
    VsgdbLog(L"EnumDebugPrograms::GetCount");

    if (Count == nullptr)
    {
        return E_POINTER;
    }

    *Count =
        static_cast<ULONG>(Programs_.size());

    return S_OK;
}
