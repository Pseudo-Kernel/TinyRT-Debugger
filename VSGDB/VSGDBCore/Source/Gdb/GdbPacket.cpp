#include "GdbPacket.h"

namespace VSGDBCore
{
    static char
    ToHexDigit(
        U8 Value)
    {
        Value &= 0xF;

        if (Value < 10)
        {
            return static_cast<char>('0' + Value);
        }

        return static_cast<char>('a' + (Value - 10));
    }

    static bool
    FromHexDigit(
        char Character,
        U8& OutValue)
    {
        if (Character >= '0' && Character <= '9')
        {
            OutValue = static_cast<U8>(Character - '0');
            return true;
        }

        if (Character >= 'a' && Character <= 'f')
        {
            OutValue = static_cast<U8>(Character - 'a' + 10);
            return true;
        }

        if (Character >= 'A' && Character <= 'F')
        {
            OutValue = static_cast<U8>(Character - 'A' + 10);
            return true;
        }

        return false;
    }

    U8
    ComputeGdbChecksum(
        const std::string& Payload)
    {
        U8 Checksum = 0;

        for (unsigned char Character : Payload)
        {
            Checksum = static_cast<U8>(Checksum + Character);
        }

        return Checksum;
    }

    std::string
    EncodeGdbPacket(
        const std::string& Payload)
    {
        const U8 Checksum = ComputeGdbChecksum(Payload);

        std::string Packet;
        Packet.reserve(Payload.size() + 4);

        Packet.push_back('$');
        Packet += Payload;
        Packet.push_back('#');
        Packet.push_back(ToHexDigit(static_cast<U8>(Checksum >> 4)));
        Packet.push_back(ToHexDigit(Checksum));

        return Packet;
    }

    bool
    DecodeHexByte(
        char High,
        char Low,
        U8& OutValue)
    {
        U8 HighValue = 0;
        U8 LowValue = 0;

        if (!FromHexDigit(High, HighValue) ||
            !FromHexDigit(Low, LowValue))
        {
            return false;
        }

        OutValue = static_cast<U8>((HighValue << 4) | LowValue);
        return true;
    }

    std::string
        EncodeHexBytes(
            const ByteVector& Bytes)
    {
        std::string Text;
        Text.reserve(Bytes.size() * 2);

        for (U8 Byte : Bytes)
        {
            Text.push_back(ToHexDigit(static_cast<U8>(Byte >> 4)));
            Text.push_back(ToHexDigit(Byte));
        }

        return Text;
    }

    Result<ByteVector>
        DecodeHexBytes(
            const std::string& HexText)
    {
        if ((HexText.size() & 1) != 0)
        {
            return Result<ByteVector>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"Hex byte text has odd length."));
        }

        ByteVector Bytes;
        Bytes.reserve(HexText.size() / 2);

        for (size_t Offset = 0; Offset < HexText.size(); Offset += 2)
        {
            U8 Byte = 0;

            if (!DecodeHexByte(
                HexText[Offset],
                HexText[Offset + 1],
                Byte))
            {
                return Result<ByteVector>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Hex byte text contains invalid character."));
            }

            Bytes.push_back(Byte);
        }

        return Result<ByteVector>::Success(Bytes);
    }
}