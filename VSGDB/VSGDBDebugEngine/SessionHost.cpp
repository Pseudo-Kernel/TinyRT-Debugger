// VSGDBDebugEngine/SessionHost.cpp

#include "SessionHost.h"

#include <VSGDBCore/Components.h>

SessionHost::SessionHost()
    : Session(VSGDBCore::CreateDefaultDebugSession())
{
}

VSGDBCore::DebugError
SessionHost::Connect(
    const std::wstring& Host,
    VSGDBCore::U16 Port)
{
    VSGDBCore::DebugSessionConnectOptions Options;
    Options.Host = Host;
    Options.Port = Port;

    return Session->Connect(Options);
}

VSGDBCore::DebugError
SessionHost::Disconnect()
{
    return Session->Disconnect();
}

VSGDBCore::IDebugSession*
SessionHost::GetSession()
{
    return Session.get();
}