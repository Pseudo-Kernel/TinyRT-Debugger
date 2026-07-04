#pragma once

#include <VSGDBCore/GdbRemoteTarget.h>

#include <string>

bool ParseU64(
    const std::wstring& Text,
    VSGDBCore::U64& OutValue);

VSGDBCore::Expected<VSGDBCore::U64> EvaluateSimpleExpression(
    VSGDBCore::GdbRemoteTarget& Target,
    VSGDBCore::U32 CpuId,
    const std::wstring& Expression);