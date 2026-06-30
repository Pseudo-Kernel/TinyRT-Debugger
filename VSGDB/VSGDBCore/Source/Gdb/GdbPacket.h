#pragma once

#include <string>

#include "../../Include/VSGDBCore/Types.h"
#include "../../Include/VSGDBCore/Result.h"

namespace VSGDBCore
{
    U8 ComputeGdbChecksum(
        const std::string& Payload);

    std::string EncodeGdbPacket(
        const std::string& Payload);

    bool DecodeHexByte(
        char High,
        char Low,
        U8& OutValue);

    std::string EncodeHexBytes(
        const ByteVector& Bytes);

    Result<ByteVector> DecodeHexBytes(
        const std::string& HexText);
}
