#pragma once

#include "Types.h"
#include "Result.h"

#include <string>

namespace VSGDBCore
{
    class GdbRemoteConnection
    {
    public:
        GdbRemoteConnection();
        ~GdbRemoteConnection();

        GdbRemoteConnection(const GdbRemoteConnection&) = delete;
        GdbRemoteConnection& operator=(const GdbRemoteConnection&) = delete;

        DebugError Connect(
            const std::wstring& Host,
            U16 Port);

        DebugError Disconnect();

        bool IsConnected() const;

        Expected<std::string> SendCommand(
            const std::string& Payload);

        DebugError SendInterrupt();

        Expected<std::string> ReceiveResponsePacket();

    private:
        DebugError InitializeWinsock();
        DebugError SendRaw(
            const char* Buffer,
            size_t Size);

        Expected<char> ReceiveChar();
        Expected<std::string> ReceivePacket();

    private:
        uintptr_t SocketValue_ = static_cast<uintptr_t>(~0ull);
        bool WinsockInitialized_ = false;
        bool Connected_ = false;
    };
}