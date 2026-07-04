
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <VSGDBCore/GdbRemoteConnection.h>

#include "GdbPacket.h"

#include <string>

#pragma comment(lib, "Ws2_32.lib")

namespace VSGDBCore
{
    static SOCKET
        ToSocket(
            uintptr_t SocketValue)
    {
        return static_cast<SOCKET>(SocketValue);
    }

    static uintptr_t
        FromSocket(
            SOCKET Socket)
    {
        return static_cast<uintptr_t>(Socket);
    }

    static std::string
        WideToUtf8(
            const std::wstring& Text)
    {
        if (Text.empty())
        {
            return {};
        }

        const int Required = WideCharToMultiByte(
            CP_UTF8,
            0,
            Text.c_str(),
            static_cast<int>(Text.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (Required <= 0)
        {
            return {};
        }

        std::string Result;
        Result.resize(static_cast<size_t>(Required));

        WideCharToMultiByte(
            CP_UTF8,
            0,
            Text.c_str(),
            static_cast<int>(Text.size()),
            const_cast<char*>(Result.data()),
            Required,
            nullptr,
            nullptr);

        return Result;
    }

    static DebugError
        MakeWinsockError(
            ErrorCode Code,
            const std::wstring& Message)
    {
        return DebugError::Failure(
            Code,
            Message,
            static_cast<U32>(WSAGetLastError()));
    }

    GdbRemoteConnection::GdbRemoteConnection()
    {
    }

    GdbRemoteConnection::~GdbRemoteConnection()
    {
        Disconnect();
    }

    DebugError
        GdbRemoteConnection::InitializeWinsock()
    {
        if (WinsockInitialized)
        {
            return DebugError::Success();
        }

        WSADATA WsaData{};
        const int Result = WSAStartup(
            MAKEWORD(2, 2),
            &WsaData);

        if (Result != 0)
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"WSAStartup failed.",
                static_cast<U32>(Result));
        }

        WinsockInitialized = true;
        return DebugError::Success();
    }

    DebugError
        GdbRemoteConnection::Connect(
            const std::wstring& Host,
            U16 Port)
    {
        if (Connected)
        {
            return DebugError::Success();
        }

        DebugError Error = InitializeWinsock();
        if (!Error.IsSuccess())
        {
            return Error;
        }

        const std::string HostUtf8 = WideToUtf8(Host);
        if (HostUtf8.empty())
        {
            return DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Host is empty or cannot be converted.");
        }

        addrinfo Hints{};
        Hints.ai_family = AF_UNSPEC;
        Hints.ai_socktype = SOCK_STREAM;
        Hints.ai_protocol = IPPROTO_TCP;

        char PortText[16]{};
        sprintf_s(
            PortText,
            "%u",
            static_cast<unsigned>(Port));

        addrinfo* AddressList = nullptr;

        int Result = getaddrinfo(
            HostUtf8.c_str(),
            PortText,
            &Hints,
            &AddressList);

        if (Result != 0)
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"getaddrinfo failed.",
                static_cast<U32>(Result));
        }

        SOCKET ConnectedSocket = INVALID_SOCKET;

        for (addrinfo* Address = AddressList;
            Address != nullptr;
            Address = Address->ai_next)
        {
            SOCKET Candidate = socket(
                Address->ai_family,
                Address->ai_socktype,
                Address->ai_protocol);

            if (Candidate == INVALID_SOCKET)
            {
                continue;
            }

            Result = connect(
                Candidate,
                Address->ai_addr,
                static_cast<int>(Address->ai_addrlen));

            if (Result == 0)
            {
                ConnectedSocket = Candidate;
                break;
            }

            closesocket(Candidate);
        }

        freeaddrinfo(AddressList);

        if (ConnectedSocket == INVALID_SOCKET)
        {
            return MakeWinsockError(
                ErrorCode::BackendFailure,
                L"Failed to connect to GDB remote target.");
        }

        SocketValue = FromSocket(ConnectedSocket);
        Connected = true;

        return DebugError::Success();
    }

    DebugError
        GdbRemoteConnection::Disconnect()
    {
        if (Connected)
        {
            shutdown(
                ToSocket(SocketValue),
                SD_BOTH);

            closesocket(ToSocket(SocketValue));

            SocketValue = FromSocket(INVALID_SOCKET);
            Connected = false;
        }

        if (WinsockInitialized)
        {
            WSACleanup();
            WinsockInitialized = false;
        }

        return DebugError::Success();
    }

    bool
        GdbRemoteConnection::IsConnected() const
    {
        return Connected;
    }

    DebugError
        GdbRemoteConnection::SendRaw(
            const char* Buffer,
            size_t Size)
    {
        size_t SentTotal = 0;

        while (SentTotal < Size)
        {
            const int Remaining = static_cast<int>(Size - SentTotal);

            const int Sent = send(
                ToSocket(SocketValue),
                Buffer + SentTotal,
                Remaining,
                0);

            if (Sent == SOCKET_ERROR)
            {
                return MakeWinsockError(
                    ErrorCode::BackendFailure,
                    L"send failed.");
            }

            if (Sent == 0)
            {
                return DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"send returned zero.");
            }

            SentTotal += static_cast<size_t>(Sent);
        }

        return DebugError::Success();
    }

    Expected<char>
        GdbRemoteConnection::ReceiveChar()
    {
        char Character = 0;

        const int Received = recv(
            ToSocket(SocketValue),
            &Character,
            1,
            0);

        if (Received == SOCKET_ERROR)
        {
            return Expected<char>::Failure(
                MakeWinsockError(
                    ErrorCode::BackendFailure,
                    L"recv failed."));
        }

        if (Received == 0)
        {
            return Expected<char>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"Connection closed by remote target."));
        }

        return Expected<char>::Success(Character);
    }

    Expected<std::string>
        GdbRemoteConnection::ReceivePacket()
    {
        for (;;)
        {
            Expected<char> StartResult = ReceiveChar();
            if (!StartResult.HasValue())
            {
                return Expected<std::string>::Failure(StartResult.Error);
            }

            if (StartResult.Value == '$')
            {
                break;
            }

            //
            // Ignore async '+'/'-' ack characters or any banner noise.
            //
        }

        std::string Payload;

        for (;;)
        {
            Expected<char> CharacterResult = ReceiveChar();
            if (!CharacterResult.HasValue())
            {
                return Expected<std::string>::Failure(CharacterResult.Error);
            }

            const char Character = CharacterResult.Value;

            if (Character == '#')
            {
                break;
            }

            Payload.push_back(Character);
        }

        Expected<char> HighResult = ReceiveChar();
        if (!HighResult.HasValue())
        {
            return Expected<std::string>::Failure(HighResult.Error);
        }

        Expected<char> LowResult = ReceiveChar();
        if (!LowResult.HasValue())
        {
            return Expected<std::string>::Failure(LowResult.Error);
        }

        U8 ReceivedChecksum = 0;
        if (!DecodeHexByte(
            HighResult.Value,
            LowResult.Value,
            ReceivedChecksum))
        {
            return Expected<std::string>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"Invalid GDB packet checksum text."));
        }

        const U8 ActualChecksum = ComputeGdbChecksum(Payload);

        if (ReceivedChecksum != ActualChecksum)
        {
            const char Nak = '-';
            SendRaw(&Nak, 1);

            return Expected<std::string>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB packet checksum mismatch."));
        }

        const char Ack = '+';
        DebugError AckError = SendRaw(&Ack, 1);
        if (!AckError.IsSuccess())
        {
            return Expected<std::string>::Failure(AckError);
        }

        return Expected<std::string>::Success(Payload);
    }

    Expected<std::string>
        GdbRemoteConnection::SendCommand(
            const std::string& Payload)
    {
        if (!Connected)
        {
            return Expected<std::string>::Failure(
                DebugError::Failure(
                    ErrorCode::NotConnected,
                    L"GDB remote connection is not connected."));
        }

        const std::string Packet = EncodeGdbPacket(Payload);

        DebugError SendError = SendRaw(
            Packet.data(),
            Packet.size());

        if (!SendError.IsSuccess())
        {
            return Expected<std::string>::Failure(SendError);
        }

        Expected<char> AckResult = ReceiveChar();
        if (!AckResult.HasValue())
        {
            return Expected<std::string>::Failure(AckResult.Error);
        }

        if (AckResult.Value == '-')
        {
            return Expected<std::string>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target rejected packet."));
        }

        if (AckResult.Value != '+')
        {
            return Expected<std::string>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target did not acknowledge packet."));
        }

        return ReceivePacket();
    }

    DebugError
        GdbRemoteConnection::SendInterrupt()
    {
        if (!Connected)
        {
            return DebugError::Failure(
                ErrorCode::NotConnected,
                L"GDB remote connection is not connected.");
        }

        const char Interrupt = 0x03;
        return SendRaw(&Interrupt, 1);
    }

    Expected<std::string>
        GdbRemoteConnection::ReceiveResponsePacket()
    {
        if (!Connected)
        {
            return Expected<std::string>::Failure(
                DebugError::Failure(
                    ErrorCode::NotConnected,
                    L"GDB remote connection is not connected."));
        }

        return ReceivePacket();
    }
}
