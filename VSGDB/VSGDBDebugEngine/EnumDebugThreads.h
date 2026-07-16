#pragma once

#include <Windows.h>
#include <Unknwn.h>
#include <msdbg.h>

#include <atomic>
#include <vector>

class EnumDebugThreads final :
    public IEnumDebugThreads2
{
public:
    explicit EnumDebugThreads(
        const std::vector<IDebugThread2*>& Threads);

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE Next(
        ULONG Count,
        IDebugThread2** Threads,
        ULONG* Fetched) override;

    HRESULT STDMETHODCALLTYPE Skip(
        ULONG Count) override;

    HRESULT STDMETHODCALLTYPE Reset() override;

    HRESULT STDMETHODCALLTYPE Clone(
        IEnumDebugThreads2** Enum) override;

    HRESULT STDMETHODCALLTYPE GetCount(
        ULONG* Count) override;

private:
    ~EnumDebugThreads();

    std::atomic<ULONG> ReferenceCount_ = 1;
    std::vector<IDebugThread2*> Threads_;
    ULONG Index_ = 0;
};