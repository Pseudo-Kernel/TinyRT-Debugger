#pragma once

#include "Types.h"
#include "RegisterTypes.h"

namespace VSGDBCore
{
    struct RegisterContext
    {
        U64 Rax = 0;
        U64 Rbx = 0;
        U64 Rcx = 0;
        U64 Rdx = 0;
        U64 Rsi = 0;
        U64 Rdi = 0;
        U64 Rbp = 0;
        U64 Rsp = 0;

        U64 R8 = 0;
        U64 R9 = 0;
        U64 R10 = 0;
        U64 R11 = 0;
        U64 R12 = 0;
        U64 R13 = 0;
        U64 R14 = 0;
        U64 R15 = 0;

        U64 Rip = 0;
        U64 Rflags = 0;

        U64 Cs = 0;
        U64 Ss = 0;
        U64 Ds = 0;
        U64 Es = 0;
        U64 Fs = 0;
        U64 Gs = 0;

        U64 Cr0 = 0;
        U64 Cr2 = 0;
        U64 Cr3 = 0;
        U64 Cr4 = 0;
        U64 Cr8 = 0;

        U64 Dr0 = 0;
        U64 Dr1 = 0;
        U64 Dr2 = 0;
        U64 Dr3 = 0;
        U64 Dr6 = 0;
        U64 Dr7 = 0;

        U64 FsBase = 0;
        U64 GsBase = 0;
        U64 KernelGsBase = 0;

        U64 Efer = 0;
    };
}