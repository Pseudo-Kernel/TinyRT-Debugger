#include "ZydisDisassembler.h"

#include <algorithm>
#include <cwchar>
#include <string>

namespace VSGDBCore
{
    static std::wstring
        Utf8ToWide(
            const char* Text)
    {
        std::wstring Result;

        if (Text == nullptr)
        {
            return Result;
        }

        while (*Text != '\0')
        {
            Result.push_back(
                static_cast<unsigned char>(*Text));

            ++Text;
        }

        return Result;
    }

    static void
        SplitInstructionText(
            const std::wstring& Text,
            std::wstring& OutMnemonic,
            std::wstring& OutOperands)
    {
        const size_t Space = Text.find(L' ');

        if (Space == std::wstring::npos)
        {
            OutMnemonic = Text;
            OutOperands.clear();
            return;
        }

        OutMnemonic = Text.substr(0, Space);

        size_t OperandStart = Space;
        while (OperandStart < Text.size() &&
            Text[OperandStart] == L' ')
        {
            ++OperandStart;
        }

        OutOperands = Text.substr(OperandStart);
    }

    ZydisDisassembler::ZydisDisassembler()
    {
        (void)SetArchitecture(ProcessorArchitecture::X64);
    }

    ProcessorArchitecture
        ZydisDisassembler::GetArchitecture() const
    {
        return Architecture;
    }

    DebugError
        ZydisDisassembler::SetArchitecture(
            ProcessorArchitecture NewArchitecture)
    {
        if (NewArchitecture != ProcessorArchitecture::X64)
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Only x64 disassembly is currently supported.");
        }

        ZyanStatus Status = ZydisDecoderInit(
            &Decoder,
            ZYDIS_MACHINE_MODE_LONG_64,
            ZYDIS_STACK_WIDTH_64);

        if (!ZYAN_SUCCESS(Status))
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"ZydisDecoderInit failed.");
        }

        Status = ZydisFormatterInit(
            &Formatter,
            ZYDIS_FORMATTER_STYLE_INTEL);

        if (!ZYAN_SUCCESS(Status))
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"ZydisFormatterInit failed.");
        }

        Architecture = NewArchitecture;
        return DebugError::Success();
    }

    Expected<std::vector<DisassembledInstruction>>
        ZydisDisassembler::Disassemble(
            U64 Address,
            const ByteVector& Bytes,
            U32 MaxInstructionCount)
    {
        std::vector<DisassembledInstruction> Instructions;

        if (MaxInstructionCount == 0)
        {
            return Expected<std::vector<DisassembledInstruction>>::Success(
                Instructions);
        }

        U64 CurrentAddress = Address;
        size_t Offset = 0;

        while (Offset < Bytes.size() &&
            Instructions.size() < MaxInstructionCount)
        {
            ZydisDecodedInstruction DecodedInstruction{};
            ZydisDecodedOperand DecodedOperands[ZYDIS_MAX_OPERAND_COUNT]{};

            const ZyanStatus DecodeStatus =
                ZydisDecoderDecodeFull(
                    &Decoder,
                    Bytes.data() + Offset,
                    Bytes.size() - Offset,
                    &DecodedInstruction,
                    DecodedOperands);

            DisassembledInstruction Instruction{};
            Instruction.Address = CurrentAddress;

            if (!ZYAN_SUCCESS(DecodeStatus))
            {
                Instruction.Size = 1;
                Instruction.Bytes.push_back(Bytes[Offset]);

                wchar_t TextBuffer[32]{};
                swprintf_s(
                    TextBuffer,
                    L"db 0x%02x",
                    static_cast<unsigned>(Bytes[Offset]));

                wchar_t OperandBuffer[16]{};
                swprintf_s(
                    OperandBuffer,
                    L"0x%02x",
                    static_cast<unsigned>(Bytes[Offset]));

                Instruction.Mnemonic = L"db";
                Instruction.Operands = OperandBuffer;
                Instruction.Text = TextBuffer;

                Instructions.push_back(std::move(Instruction));

                ++Offset;
                ++CurrentAddress;
                continue;
            }

            Instruction.Size =
                static_cast<U32>(DecodedInstruction.length);

            const size_t CopySize =
                std::min<size_t>(
                    DecodedInstruction.length,
                    Bytes.size() - Offset);

            Instruction.Bytes.insert(
                Instruction.Bytes.end(),
                Bytes.begin() + Offset,
                Bytes.begin() + Offset + CopySize);

            char TextBuffer[256]{};

            const ZyanStatus FormatStatus =
                ZydisFormatterFormatInstruction(
                    &Formatter,
                    &DecodedInstruction,
                    DecodedOperands,
                    DecodedInstruction.operand_count_visible,
                    TextBuffer,
                    sizeof(TextBuffer),
                    CurrentAddress,
                    nullptr);

            if (ZYAN_SUCCESS(FormatStatus))
            {
                Instruction.Text = Utf8ToWide(TextBuffer);

                SplitInstructionText(
                    Instruction.Text,
                    Instruction.Mnemonic,
                    Instruction.Operands);
            }
            else
            {
                Instruction.Mnemonic = L"<format>";
                Instruction.Operands = L"failed";
                Instruction.Text = L"<format failed>";
            }

            Instructions.push_back(std::move(Instruction));

            Offset += DecodedInstruction.length;
            CurrentAddress += DecodedInstruction.length;
        }

        return Expected<std::vector<DisassembledInstruction>>::Success(
            Instructions);
    }
}