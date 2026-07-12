#pragma once

#include <Windows.h>
#include <Unknwn.h>
#include <OleAuto.h>
#include <msdbg.h>

#include <atomic>

class DebugProgramNode final :
    public IDebugProgramNode2
{
public:
    DebugProgramNode();

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID InterfaceId,
        void** Object) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    HRESULT STDMETHODCALLTYPE GetProgramName(
        BSTR* ProgramName) override;

    HRESULT STDMETHODCALLTYPE GetHostName(
        DWORD HostNameType,
        BSTR* HostName) override;

    HRESULT STDMETHODCALLTYPE GetHostPid(
        AD_PROCESS_ID* HostProcessId) override;

    HRESULT STDMETHODCALLTYPE GetHostMachineName_V7(
        BSTR* HostMachineName) override;

    HRESULT STDMETHODCALLTYPE Attach_V7(
        IDebugProgram2* Program,
        IDebugEventCallback2* Callback,
        DWORD Reason) override;

    HRESULT STDMETHODCALLTYPE GetEngineInfo(
        BSTR* EngineName,
        GUID* EngineGuid) override;

    HRESULT STDMETHODCALLTYPE DetachDebugger_V7() override;

private:
    ~DebugProgramNode();


    std::atomic<ULONG> ReferenceCount_ = 1;
};