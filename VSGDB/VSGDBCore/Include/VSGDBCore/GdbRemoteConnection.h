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

        Result<std::string> SendCommand(
            const std::string& Payload);

        DebugError SendInterrupt();

        Result<std::string> ReceiveResponsePacket();

    private:
        DebugError InitializeWinsock();
        DebugError SendRaw(
            const char* Buffer,
            size_t Size);

        Result<char> ReceiveChar();
        Result<std::string> ReceivePacket();

    private:
        uintptr_t SocketValue = static_cast<uintptr_t>(~0ull);
        bool WinsockInitialized = false;
        bool Connected = false;
    };
}