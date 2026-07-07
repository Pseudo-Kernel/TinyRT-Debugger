#pragma once

#include "Types.h"

#include <vector>

namespace VSGDBCore
{
    struct StackFrame
    {
        U32 Index = 0;

        U64 InstructionPointer = 0;
        U64 StackPointer = 0;
        U64 FramePointer = 0;

        U64 EstablisherFrame = 0;
        U64 ReturnAddressLocation = 0;
        U64 ReturnAddress = 0;

        bool IsCurrentFrame = false;
        bool IsLeafFunction = false;
    };

    struct StackWalkOptions
    {
        U32 MaxFrames = 64;

        //
        // Conservative sanity bounds.
        //
        U64 MaxFrameDistance = 1024 * 1024;
    };
}