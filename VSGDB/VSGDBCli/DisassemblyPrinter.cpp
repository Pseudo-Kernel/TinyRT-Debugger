#include "DisassemblyPrinter.h"

#include <cstdio>

static void
PrintInstructionBytes(
    const VSGDBCore::ByteVector& Bytes)
{
    constexpr size_t ByteColumnWidth = 16;

    for (size_t Index = 0; Index < ByteColumnWidth; ++Index)
    {
        if (Index < Bytes.size())
        {
            std::printf("%02x ", Bytes[Index]);
        }
        else
        {
            std::printf("   ");
        }
    }
}

static std::wstring
FormatBranchTargetAnnotation(
    const VSGDBCore::DisassembledInstruction& Instruction,
    const AddressFormatter& FormatAddress)
{
    if (!Instruction.HasBranchTarget || !FormatAddress)
    {
        return {};
    }

    const AddressLabel Label =
        FormatAddress(Instruction.BranchTarget);

    if (Label.Text.empty())
    {
        return {};
    }

    std::wstring Text = L" <";
    Text += Label.Text;
    Text += L">";

    return Text;
}

void
PrintDisassembly(
    const std::vector<VSGDBCore::DisassembledInstruction>& Instructions,
    VSGDBCore::U64 CurrentRip,
    const AddressFormatter& FormatAddress)
{
    std::wstring LastLabelKey;

    for (const auto& Instruction : Instructions)
    {
        if (FormatAddress)
        {
            const AddressLabel Label =
                FormatAddress(Instruction.Address);

            if (!Label.Key.empty() &&
                Label.Key != LastLabelKey)
            {
                std::wprintf(
                    L"%s:\n",
                    Label.Text.c_str());

                LastLabelKey = Label.Key;
            }
            else if (Label.Key.empty())
            {
                LastLabelKey.clear();
            }
        }

        std::wprintf(
            L"%s %016llx  ",
            Instruction.Address == CurrentRip ? L"=>" : L"  ",
            Instruction.Address);

        for (size_t Index = 0; Index < Instruction.Bytes.size(); ++Index)
        {
            std::wprintf(
                L"%02x ",
                Instruction.Bytes[Index]);
        }

        for (size_t Index = Instruction.Bytes.size(); Index < 10; ++Index)
        {
            std::wprintf(L"   ");
        }

        const std::wstring BranchAnnotation =
            FormatBranchTargetAnnotation(
                Instruction,
                FormatAddress);

        std::wprintf(
            L" %s%s\n",
            Instruction.Text.c_str(),
            BranchAnnotation.c_str());
    }
}
