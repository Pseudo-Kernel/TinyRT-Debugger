#pragma once

#include <string>

#include "../../Include/VSGDBCore/RegisterContext.h"
#include "../../Include/VSGDBCore/Result.h"

namespace VSGDBCore
{
    Result<RegisterContext> DecodeX64GdbRegisters(
        const std::string& HexText);

    Result<U64> DecodeGdbRegisterValueToU64(
        const std::string& HexText,
        U32 BitSize);
}