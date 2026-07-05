#pragma once

#include "DisassemblyPrinter.h"

#include <VSGDBCore/IModuleManager.h>
#include <VSGDBCore/ISymbolManager.h>
#include <VSGDBCore/RegisterContext.h>
#include <VSGDBCore/X64Paging.h>

#include <string>

class DebugTextFormatter final
{
public:
    DebugTextFormatter(
        const VSGDBCore::IModuleManager* ModuleManager,
        const VSGDBCore::ISymbolManager* SymbolManager);

    //
    // Address formatting.
    //
    AddressLabel FormatAddressLabel(
        VSGDBCore::U64 Address) const;

    std::wstring FormatAddressInline(
        VSGDBCore::U64 Address) const;

    //
    // Registers.
    //
    void PrintRegister(
        const wchar_t* Name,
        VSGDBCore::U64 Value) const;

    void PrintRegisterRaw(
        const wchar_t* Name,
        VSGDBCore::U64 Value) const;

    void PrintRegisters(
        const VSGDBCore::RegisterContext& Registers) const;

    bool PrintRegisterByName(
        const VSGDBCore::RegisterContext& Registers,
        const std::wstring& Name) const;

    //
    // Paging / fault output.
    //
    void PrintTranslation(
        const VSGDBCore::X64AddressTranslationResult& Translation) const;

    void PrintPageFaultAnalysis(
        const VSGDBCore::X64PageFaultAnalysis& Analysis) const;

private:
    static std::wstring NormalizeName(
        const std::wstring& Name);

    static bool IsSymbolAwareRegister(
        const std::wstring& Name);

    AddressLabel FormatAddressWithSymbol(
        VSGDBCore::U64 Address) const;

    AddressLabel FormatAddressWithModule(
        VSGDBCore::U64 Address) const;

    const VSGDBCore::IModuleManager* ModuleManager = nullptr;
    const VSGDBCore::ISymbolManager* SymbolManager = nullptr;
};