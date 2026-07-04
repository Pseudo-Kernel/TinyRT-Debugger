#pragma once

#include <VSGDBCore/IDisassembler.h>

#include <functional>
#include <string>
#include <vector>

struct AddressLabel
{
    std::wstring Key;
    std::wstring Text;
};

using AddressFormatter =
    std::function<AddressLabel(VSGDBCore::U64 Address)>;

void
PrintDisassembly(
    const std::vector<VSGDBCore::DisassembledInstruction>& Instructions,
    VSGDBCore::U64 CurrentRip,
    const AddressFormatter& FormatAddress);
