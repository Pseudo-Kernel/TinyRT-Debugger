#include "X64FramePointerStackWalker.h"

#include <VSGDBCore/IDebugSession.h>
#include <VSGDBCore/RegisterContext.h>

#include <cstring>

namespace VSGDBCore
{
    bool
        X64FramePointerStackWalker::IsCanonicalAddress(
            U64 Address)
    {
        //
        // 48-bit canonical address check.
        //
        const U64 Sign = Address & (1ull << 47);
        const U64 High = Address >> 48;

        if (Sign)
        {
            return High == 0xffff;
        }

        return High == 0;
    }

    bool
        X64FramePointerStackWalker::IsReasonableNextFramePointer(
            U64 CurrentFramePointer,
            U64 NextFramePointer,
            U64 MaxFrameDistance)
    {
        if (NextFramePointer == 0)
        {
            return false;
        }

        if (!IsCanonicalAddress(NextFramePointer))
        {
            return false;
        }

        //
        // x64 stack grows downward. Caller RBP should normally be above
        // callee RBP.
        //
        if (NextFramePointer <= CurrentFramePointer)
        {
            return false;
        }

        if ((NextFramePointer - CurrentFramePointer) > MaxFrameDistance)
        {
            return false;
        }

        //
        // RBP should be naturally aligned.
        //
        if ((NextFramePointer & 0x7) != 0)
        {
            return false;
        }

        return true;
    }

    Expected<std::vector<StackFrame>>
        X64FramePointerStackWalker::WalkStack(
            IDebugSession& Session,
            U32 CpuId,
            const StackWalkOptions& Options)
    {
        auto Registers = Session.GetRegisters(CpuId);

        if (!Registers.HasValue())
        {
            return Expected<std::vector<StackFrame>>::Failure(
                Registers.Error);
        }

        std::vector<StackFrame> Frames;

        StackFrame Current{};
        Current.Index = 0;
        Current.InstructionPointer = Registers.Value.Rip;
        Current.FramePointer = Registers.Value.Rbp;
        Current.IsCurrentFrame = true;

        Frames.push_back(Current);

        U64 FramePointer = Registers.Value.Rbp;

        for (U32 Index = 1; Index < Options.MaxFrames; ++Index)
        {
            if (FramePointer == 0)
            {
                break;
            }

            if (!IsCanonicalAddress(FramePointer))
            {
                break;
            }

            if ((FramePointer & 0x7) != 0)
            {
                break;
            }

            auto Bytes = Session.ReadVirtualMemory(
                CpuId,
                FramePointer,
                16);

            if (!Bytes.HasValue())
            {
                break;
            }

            if (Bytes.Value.size() < 16)
            {
                break;
            }

            U64 NextFramePointer = 0;
            U64 ReturnAddress = 0;

            std::memcpy(
                &NextFramePointer,
                Bytes.Value.data(),
                sizeof(NextFramePointer));

            std::memcpy(
                &ReturnAddress,
                Bytes.Value.data() + 8,
                sizeof(ReturnAddress));

            if (!IsCanonicalAddress(ReturnAddress))
            {
                break;
            }

            StackFrame Frame{};
            Frame.Index = Index;
            Frame.InstructionPointer = ReturnAddress;
            Frame.FramePointer = FramePointer;
            Frame.ReturnAddress = ReturnAddress;
            Frame.IsCurrentFrame = false;

            Frames.push_back(Frame);

            if (!IsReasonableNextFramePointer(
                FramePointer,
                NextFramePointer,
                Options.MaxFrameDistance))
            {
                break;
            }

            FramePointer = NextFramePointer;
        }

        return Expected<std::vector<StackFrame>>::Success(
            std::move(Frames));
    }
}