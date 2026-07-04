#pragma once

#include <VSGDBCore/Types.h>

void PrintHexDump(
    VSGDBCore::U64 BaseAddress,
    const VSGDBCore::ByteVector& Bytes);