#pragma once

#include <string>

#include "../../Include/VSGDBCore/RegisterContext.h"
#include "../../Include/VSGDBCore/Result.h"

namespace VSGDBCore
{
    Expected<RegisterContext> DecodeX64GdbRegisters(
        const std::string& HexText);

    Expected<U64> DecodeGdbRegisterValueToU64(
        const std::string& HexText,
        U32 BitSize);
}