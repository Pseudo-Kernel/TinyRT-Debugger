// VSGDBDebugEngine/SessionHost.h

#pragma once

#include <VSGDBCore/IDebugSession.h>

#include <memory>
#include <string>

class SessionHost final
{
public:
    SessionHost();

    VSGDBCore::DebugError Connect(
        const std::wstring& Host,
        VSGDBCore::U16 Port);

    VSGDBCore::DebugError Disconnect();

    VSGDBCore::IDebugSession* GetSession();

private:
    std::unique_ptr<VSGDBCore::IDebugSession> Session;
};