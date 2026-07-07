#pragma once

#include <VSGDBCore/Types.h>
#include <VSGDBCore/RegisterContext.h>
#include <VSGDBCore/ModuleTypes.h>

#include <vector>

namespace VSGDBCore
{
    enum class X64UnwindOp : U8
    {
        PushNonVol = 0,
        AllocLarge = 1,
        AllocSmall = 2,
        SetFpReg = 3,
        SaveNonVol = 4,
        SaveNonVolFar = 5,
        Epilog = 6,
        Spare = 7,
        SaveXmm128 = 8,
        SaveXmm128Far = 9,
        PushMachFrame = 10
    };

    struct X64RuntimeFunction
    {
        U32 BeginAddress = 0;
        U32 EndAddress = 0;
        U32 UnwindInfoAddress = 0;
    };

    static_assert(
        sizeof(X64RuntimeFunction) == 12,
        "X64RuntimeFunction must match IMAGE_RUNTIME_FUNCTION_ENTRY size");

    struct X64UnwindCode
    {
        U8 CodeOffset = 0;
        U8 UnwindOp = 0;
        U8 OpInfo = 0;

        U8 Raw0 = 0;
        U8 Raw1 = 0;
    };

    struct X64UnwindInfo
    {
        U8 Version = 0;
        U8 Flags = 0;
        U8 SizeOfProlog = 0;
        U8 CountOfCodes = 0;
        U8 FrameRegister = 0;
        U8 FrameOffset = 0;

        std::vector<X64UnwindCode> Codes;
    };

    struct X64UnwindModuleData
    {
        ModuleId Id = 0;
        U64 BaseAddress = 0;
        U64 Size = 0;
        std::wstring ImagePath;

        std::vector<U8> ImageBytes;
        std::vector<X64RuntimeFunction> RuntimeFunctions;
    };

    struct X64UnwindResult
    {
        RegisterContext CallerContext{};
        U64 EstablisherFrame = 0;
        U64 ReturnAddressLocation = 0;
        U64 ReturnAddress = 0;
        bool IsLeafFunction = false;
    };

}