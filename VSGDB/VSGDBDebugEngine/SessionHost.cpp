// VSGDBDebugEngine/SessionHost.cpp

#include "SessionHost.h"
#include "LogUtils.h"

#include <VSGDBCore/Components.h>

SessionHost::SessionHost()
    : Session_(VSGDBCore::CreateDefaultDebugSession())
{
    VsgdbLog(__FUNCTIONW__);
}

SessionHost::~SessionHost()
{
    VsgdbLog(__FUNCTIONW__);
}

VSGDBCore::DebugError
SessionHost::Connect(
    const std::wstring& Host,
    VSGDBCore::U16 Port)
{
    VsgdbLog(__FUNCTIONW__);

    VSGDBCore::DebugSessionConnectOptions Options;
    Options.Host = Host;
    Options.Port = Port;

    return Session_->Connect(Options);
}

VSGDBCore::DebugError
SessionHost::Disconnect()
{
    VsgdbLog(__FUNCTIONW__);

    return Session_->Disconnect();
}

VSGDBCore::IDebugSession*
SessionHost::GetSession()
{
    return Session_.get();
}
