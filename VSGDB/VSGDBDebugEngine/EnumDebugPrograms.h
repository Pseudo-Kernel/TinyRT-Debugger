#pragma once

#include <Windows.h>
#include <msdbg.h>

#include <atomic>
#include <vector>

class EnumDebugPrograms final :
    public IEnumDebugPrograms2
{
public:
    explicit EnumDebugPrograms(
        const std::vector<IDebugProgram2*>& Programs);

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE Next(
        ULONG Count,
        IDebugProgram2** Programs,
        ULONG* Fetched) override;

    HRESULT STDMETHODCALLTYPE Skip(
        ULONG Count) override;

    HRESULT STDMETHODCALLTYPE Reset() override;

    HRESULT STDMETHODCALLTYPE Clone(
        IEnumDebugPrograms2** Enum) override;

    HRESULT STDMETHODCALLTYPE GetCount(
        ULONG* Count) override;

private:
    ~EnumDebugPrograms();

    std::atomic<ULONG> ReferenceCount_ = 1;
    std::vector<IDebugProgram2*> Programs_;
    ULONG Index_ = 0;
};