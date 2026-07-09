#pragma once

#include <VSGDBCore/IDebugSession.h>
#include <VSGDBCore/ISymbolManager.h>

#include <string>

bool ParseU64(
    const std::wstring& Text,
    VSGDBCore::U64& OutValue);

VSGDBCore::Expected<VSGDBCore::U64> EvaluateSimpleExpression(
    VSGDBCore::IDebugSession& Session,
    VSGDBCore::U32 CpuId,
    const std::wstring& Expression,
    const VSGDBCore::ISymbolManager* SymbolManager);
