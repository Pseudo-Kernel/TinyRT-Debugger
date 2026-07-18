
#include "DebugEvents.h"
#include "LogUtils.h"

// **************************************** //
//          DebugEngineCreateEvent
// **************************************** //

DebugEngineCreateEvent::DebugEngineCreateEvent(
    IDebugEngine2* Engine)
{
    VsgdbLog(L"DebugEngineCreateEvent created");

    Engine_ = Engine;

    if (Engine_ != nullptr)
    {
        Engine_->AddRef();
    }
}

DebugEngineCreateEvent::~DebugEngineCreateEvent()
{
    VsgdbLog(L"DebugEngineCreateEvent destroyed");

    if (Engine_ != nullptr)
    {
        Engine_->Release();
        Engine_ = nullptr;
    }
}

HRESULT STDMETHODCALLTYPE
DebugEngineCreateEvent::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugEvent2))
    {
        *Object = static_cast<IDebugEvent2*>(this);
        AddRef();
        return S_OK;
    }

    if (InterfaceId == __uuidof(IDebugEngineCreateEvent2))
    {
        *Object = static_cast<IDebugEngineCreateEvent2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugEngineCreateEvent::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
DebugEngineCreateEvent::Release()
{
    ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
DebugEngineCreateEvent::GetAttributes(
    DWORD* EventAttributes)
{
    VsgdbLog(L"DebugEngineCreateEvent::GetAttributes");

    if (EventAttributes == nullptr)
    {
        return E_POINTER;
    }

    *EventAttributes = EVENT_ASYNCHRONOUS;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
DebugEngineCreateEvent::GetEngine(
    IDebugEngine2** Engine)
{
    VsgdbLog(L"DebugEngineCreateEvent::GetEngine");

    if (Engine == nullptr)
    {
        return E_POINTER;
    }

    *Engine = Engine_;

    if (*Engine != nullptr)
    {
        (*Engine)->AddRef();
    }

    return S_OK;
}





// **************************************** //
//         DebugProgramCreateEvent
// **************************************** //


DebugProgramCreateEvent::DebugProgramCreateEvent()
{
    VsgdbLog(L"DebugProgramCreateEvent created");
}

DebugProgramCreateEvent::~DebugProgramCreateEvent()
{
    VsgdbLog(L"DebugProgramCreateEvent destroyed");
}

HRESULT STDMETHODCALLTYPE
DebugProgramCreateEvent::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugEvent2))
    {
        *Object = static_cast<IDebugEvent2*>(this);
        AddRef();
        return S_OK;
    }

    if (InterfaceId == __uuidof(IDebugProgramCreateEvent2))
    {
        *Object = static_cast<IDebugProgramCreateEvent2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugProgramCreateEvent::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
DebugProgramCreateEvent::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
DebugProgramCreateEvent::GetAttributes(
    DWORD* EventAttributes)
{
    VsgdbLog(L"DebugProgramCreateEvent::GetAttributes");

    if (EventAttributes == nullptr)
    {
        return E_POINTER;
    }

    *EventAttributes = EVENT_ASYNCHRONOUS;

    return S_OK;
}




// **************************************** //
//          DebugThreadCreateEvent
// **************************************** //


DebugThreadCreateEvent::DebugThreadCreateEvent()
{
    VsgdbLog(L"DebugThreadCreateEvent created");
}

DebugThreadCreateEvent::~DebugThreadCreateEvent()
{
    VsgdbLog(L"DebugThreadCreateEvent destroyed");
}

HRESULT STDMETHODCALLTYPE
DebugThreadCreateEvent::QueryInterface(
    REFIID InterfaceId,
    void** Object)
{
    if (Object == nullptr)
    {
        return E_POINTER;
    }

    *Object = nullptr;

    if (InterfaceId == __uuidof(IUnknown) ||
        InterfaceId == __uuidof(IDebugEvent2))
    {
        *Object = static_cast<IDebugEvent2*>(this);
        AddRef();
        return S_OK;
    }

    if (InterfaceId == __uuidof(IDebugThreadCreateEvent2))
    {
        *Object = static_cast<IDebugThreadCreateEvent2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
DebugThreadCreateEvent::AddRef()
{
    return ++ReferenceCount_;
}

ULONG STDMETHODCALLTYPE
DebugThreadCreateEvent::Release()
{
    const ULONG Count = --ReferenceCount_;

    if (Count == 0)
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
DebugThreadCreateEvent::GetAttributes(
    DWORD* EventAttributes)
{
    VsgdbLog(L"DebugThreadCreateEvent::GetAttributes");

    if (EventAttributes == nullptr)
    {
        return E_POINTER;
    }

    *EventAttributes = EVENT_ASYNCHRONOUS;

    return S_OK;
}
