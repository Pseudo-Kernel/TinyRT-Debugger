#include "MemoryPrinter.h"

#include <cstdio>
#include <cctype>

void
PrintHexDump(
    VSGDBCore::U64 BaseAddress,
    const VSGDBCore::ByteVector& Bytes)
{
    constexpr size_t BytesPerLine = 16;

    for (size_t Offset = 0; Offset < Bytes.size(); Offset += BytesPerLine)
    {
        const size_t Remaining = Bytes.size() - Offset;
        const size_t LineSize =
            Remaining < BytesPerLine ? Remaining : BytesPerLine;

        std::printf(
            "%016llx  ",
            BaseAddress + static_cast<VSGDBCore::U64>(Offset));

        for (size_t Index = 0; Index < BytesPerLine; ++Index)
        {
            if (Index < LineSize)
            {
                std::printf(
                    "%02x ",
                    Bytes[Offset + Index]);
            }
            else
            {
                std::printf("   ");
            }

            if (Index == 7)
            {
                std::printf(" ");
            }
        }

        std::printf(" ");

        for (size_t Index = 0; Index < LineSize; ++Index)
        {
            const unsigned char Character =
                Bytes[Offset + Index];

            std::printf(
                "%c",
                std::isprint(Character) ? Character : '.');
        }

        std::printf("\n");
    }
}