#include "GdbRegisterCodec.h"
#include "GdbPacket.h"

namespace VSGDBCore
{
    static bool
        DecodeHexByteAt(
            const std::string& Text,
            size_t Offset,
            U8& OutValue)
    {
        if (Offset + 1 >= Text.size())
        {
            return false;
        }

        return DecodeHexByte(
            Text[Offset],
            Text[Offset + 1],
            OutValue);
    }

    static bool
        ReadLittleEndianU32FromHex(
            const std::string& Text,
            size_t& Offset,
            U32& OutValue)
    {
        OutValue = 0;

        for (size_t ByteIndex = 0; ByteIndex < 4; ++ByteIndex)
        {
            U8 Byte = 0;

            if (!DecodeHexByteAt(
                Text,
                Offset,
                Byte))
            {
                return false;
            }

            OutValue |=
                static_cast<U32>(Byte)
                << (ByteIndex * 8);

            Offset += 2;
        }

        return true;
    }

    static bool
        ReadLittleEndianU64FromHex(
            const std::string& Text,
            size_t& Offset,
            U64& OutValue)
    {
        OutValue = 0;

        for (size_t ByteIndex = 0; ByteIndex < 8; ++ByteIndex)
        {
            U8 Byte = 0;

            if (!DecodeHexByteAt(
                Text,
                Offset,
                Byte))
            {
                return false;
            }

            OutValue |=
                static_cast<U64>(Byte)
                << (ByteIndex * 8);

            Offset += 2;
        }

        return true;
    }

    Result<RegisterContext>
        DecodeX64GdbRegisters(
            const std::string& HexText)
    {
        //
        // QEMU x86-64 'g' packet common leading layout:
        //
        //   rax     64
        //   rbx     64
        //   rcx     64
        //   rdx     64
        //   rsi     64
        //   rdi     64
        //   rbp     64
        //   rsp     64
        //   r8      64
        //   r9      64
        //   r10     64
        //   r11     64
        //   r12     64
        //   r13     64
        //   r14     64
        //   r15     64
        //   rip     64
        //   eflags  32
        //   cs      32
        //   ss      32
        //   ds      32
        //   es      32
        //   fs      32
        //   gs      32
        //
        // Additional registers may follow. Ignore them for now.
        //

        constexpr size_t Gpr64Count = 17; // rax..r15 + rip
        constexpr size_t Reg32Count = 7;  // eflags + cs/ss/ds/es/fs/gs

        constexpr size_t RequiredHexLength =
            (Gpr64Count * sizeof(U64) * 2) +
            (Reg32Count * sizeof(U32) * 2);

        if (HexText.size() < RequiredHexLength)
        {
            return Result<RegisterContext>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB register packet is too short."));
        }

        RegisterContext Context{};
        size_t Offset = 0;

        U32 Eflags = 0;
        U32 Cs = 0;
        U32 Ss = 0;
        U32 Ds = 0;
        U32 Es = 0;
        U32 Fs = 0;
        U32 Gs = 0;

        if (!ReadLittleEndianU64FromHex(HexText, Offset, Context.Rax) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rbx) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rcx) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rdx) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rsi) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rdi) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rbp) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rsp) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R8) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R9) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R10) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R11) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R12) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R13) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R14) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.R15) ||
            !ReadLittleEndianU64FromHex(HexText, Offset, Context.Rip) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Eflags) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Cs) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Ss) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Ds) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Es) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Fs) ||
            !ReadLittleEndianU32FromHex(HexText, Offset, Gs))
        {
            return Result<RegisterContext>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"Failed to decode GDB register packet."));
        }

        Context.Rflags = Eflags;
        Context.Cs = Cs;
        Context.Ss = Ss;
        Context.Ds = Ds;
        Context.Es = Es;
        Context.Fs = Fs;
        Context.Gs = Gs;

        return Result<RegisterContext>::Success(Context);
    }

    Result<U64>
        DecodeGdbRegisterValueToU64(
            const std::string& HexText,
            U32 BitSize)
    {
        if (BitSize == 0 || BitSize > 64)
        {
            return Result<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Unsupported register bit size."));
        }

        const size_t ByteCount = (BitSize + 7) / 8;
        const size_t RequiredHexLength = ByteCount * 2;

        if (HexText.size() < RequiredHexLength)
        {
            return Result<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB register value is too short."));
        }

        size_t Offset = 0;
        U64 Value = 0;

        for (size_t ByteIndex = 0; ByteIndex < ByteCount; ++ByteIndex)
        {
            U8 Byte = 0;

            if (!DecodeHexByteAt(HexText, Offset, Byte))
            {
                return Result<U64>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Invalid GDB register hex value."));
            }

            Value |= static_cast<U64>(Byte) << (ByteIndex * 8);
            Offset += 2;
        }

        return Result<U64>::Success(Value);
    }
}