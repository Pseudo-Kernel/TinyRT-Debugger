#include "X64PeUnwindStackWalker.h"

namespace VSGDBCore
{
    static U64
        GetRegisterByUnwindNumber(
            const RegisterContext& Context,
            U8 RegisterNumber)
    {
        switch (RegisterNumber)
        {
        case 0: return Context.Rax;
        case 1: return Context.Rcx;
        case 2: return Context.Rdx;
        case 3: return Context.Rbx;
        case 4: return Context.Rsp;
        case 5: return Context.Rbp;
        case 6: return Context.Rsi;
        case 7: return Context.Rdi;
        case 8: return Context.R8;
        case 9: return Context.R9;
        case 10: return Context.R10;
        case 11: return Context.R11;
        case 12: return Context.R12;
        case 13: return Context.R13;
        case 14: return Context.R14;
        case 15: return Context.R15;
        default: return 0;
        }
    }

    static bool
        SetRegisterByUnwindNumber(
            RegisterContext& Context,
            U8 RegisterNumber,
            U64 Value)
    {
        switch (RegisterNumber)
        {
        case 0: Context.Rax = Value; return true;
        case 1: Context.Rcx = Value; return true;
        case 2: Context.Rdx = Value; return true;
        case 3: Context.Rbx = Value; return true;
        case 4: Context.Rsp = Value; return true;
        case 5: Context.Rbp = Value; return true;
        case 6: Context.Rsi = Value; return true;
        case 7: Context.Rdi = Value; return true;
        case 8: Context.R8 = Value; return true;
        case 9: Context.R9 = Value; return true;
        case 10: Context.R10 = Value; return true;
        case 11: Context.R11 = Value; return true;
        case 12: Context.R12 = Value; return true;
        case 13: Context.R13 = Value; return true;
        case 14: Context.R14 = Value; return true;
        case 15: Context.R15 = Value; return true;
        default: return false;
        }
    }

    static Expected<U64>
        ReadU64FromTarget(
            IDebugTarget& Target,
            U32 CpuId,
            U64 Address)
    {
        auto Bytes = Target.ReadVirtualMemory(
            CpuId,
            Address,
            sizeof(U64));

        if (!Bytes.HasValue())
        {
            return Expected<U64>::Failure(
                Bytes.Error);
        }

        if (Bytes.Value.size() != sizeof(U64))
        {
            return Expected<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::MemoryReadFailure,
                    L"Short stack memory read"));
        }

        U64 Value = 0;

        std::memcpy(
            &Value,
            Bytes.Value.data(),
            sizeof(Value));

        return Expected<U64>::Success(Value);
    }

    static bool
        IsCanonicalX64Address(
            U64 Address)
    {
        const U64 High = Address >> 48;
        const bool Sign = (Address & (1ull << 47)) != 0;

        if (Sign)
        {
            return High == 0xffff;
        }

        return High == 0;
    }

    static U16
        ReadUnwindSlotU16(
            const X64UnwindCode& Slot)
    {
        return static_cast<U16>(
            Slot.Raw0 |
            (static_cast<U16>(Slot.Raw1) << 8));
    }

    X64PeUnwindStackWalker::X64PeUnwindStackWalker(
        const IModuleManager* ModuleManager)
        : ModuleManager(ModuleManager)
    {
    }

    Expected<X64UnwindResult>
        X64PeUnwindStackWalker::UnwindLeafFrame(
            IDebugTarget& Target,
            U32 CpuId,
            const RegisterContext& Context) const
    {
        auto ReturnAddress = ReadU64FromTarget(
            Target,
            CpuId,
            Context.Rsp);

        if (!ReturnAddress.HasValue())
        {
            return Expected<X64UnwindResult>::Failure(
                ReturnAddress.Error);
        }

        X64UnwindResult Result{};
        Result.CallerContext = Context;
        Result.CallerContext.Rip = ReturnAddress.Value;
        Result.CallerContext.Rsp = Context.Rsp + 8;
        Result.EstablisherFrame = Context.Rsp;
        Result.ReturnAddressLocation = Context.Rsp;
        Result.ReturnAddress = ReturnAddress.Value;
        Result.IsLeafFunction = true;

        return Expected<X64UnwindResult>::Success(Result);
    }

    Expected<X64UnwindResult>
        X64PeUnwindStackWalker::UnwindFrame(
            IDebugTarget& Target,
            U32 CpuId,
            const RegisterContext& Context)
    {
        if (ModuleManager == nullptr)
        {
            return UnwindLeafFrame(
                Target,
                CpuId,
                Context);
        }

        auto Module =
            ModuleManager->GetModuleByAddress(Context.Rip);

        if (!Module.HasValue())
        {
            return UnwindLeafFrame(
                Target,
                CpuId,
                Context);
        }

        auto RuntimeFunction =
            UnwindData.LookupRuntimeFunction(
                Module.Value,
                Context.Rip);

        if (!RuntimeFunction.HasValue())
        {
            //
            // No .pdata entry means leaf function in x64 unwinding rules.
            //
            return UnwindLeafFrame(
                Target,
                CpuId,
                Context);
        }

        auto ModuleData =
            UnwindData.GetOrLoadModule(Module.Value);

        if (!ModuleData.HasValue())
        {
            return Expected<X64UnwindResult>::Failure(
                ModuleData.Error);
        }

        auto UnwindInfo =
            UnwindData.ReadUnwindInfo(
                *ModuleData.Value,
                RuntimeFunction.Value.UnwindInfoAddress);

        if (!UnwindInfo.HasValue())
        {
            return Expected<X64UnwindResult>::Failure(
                UnwindInfo.Error);
        }

        return ApplyUnwindInfo(
            Target,
            CpuId,
            Context,
            UnwindInfo.Value);
    }

    Expected<X64UnwindResult>
        X64PeUnwindStackWalker::ApplyUnwindInfo(
            IDebugTarget& Target,
            U32 CpuId,
            const RegisterContext& Context,
            const X64UnwindInfo& UnwindInfo) const
    {
        RegisterContext Caller = Context;

        U64 FrameBase = Context.Rsp;

        if (UnwindInfo.FrameRegister != 0)
        {
            const U64 FrameRegisterValue =
                GetRegisterByUnwindNumber(
                    Context,
                    UnwindInfo.FrameRegister);

            FrameBase =
                FrameRegisterValue -
                static_cast<U64>(UnwindInfo.FrameOffset) * 16;
        }

        U64 StackPointer = FrameBase;

        //
        // Replay unwind codes in the order stored in UNWIND_INFO.
        // The array is already in reverse prologue order, which is the
        // correct order for unwinding the fully-established frame.
        //
        for (size_t Index = 0; Index < UnwindInfo.Codes.size(); ++Index)
        {
            const X64UnwindCode& Code =
                UnwindInfo.Codes[Index];

            switch (static_cast<X64UnwindOp>(Code.UnwindOp))
            {
            case X64UnwindOp::PushNonVol:
            {
                auto SavedValue = ReadU64FromTarget(
                    Target,
                    CpuId,
                    StackPointer);

                if (!SavedValue.HasValue())
                {
                    return Expected<X64UnwindResult>::Failure(
                        SavedValue.Error);
                }

                SetRegisterByUnwindNumber(
                    Caller,
                    Code.OpInfo,
                    SavedValue.Value);

                StackPointer += 8;
                break;
            }

            case X64UnwindOp::AllocSmall:
            {
                const U64 AllocationSize =
                    static_cast<U64>(Code.OpInfo) * 8 + 8;

                StackPointer += AllocationSize;
                break;
            }

            case X64UnwindOp::AllocLarge:
            {
                if (Code.OpInfo == 0)
                {
                    if (Index + 1 >= UnwindInfo.Codes.size())
                    {
                        return Expected<X64UnwindResult>::Failure(
                            DebugError::Failure(
                                ErrorCode::SymbolFailure,
                                L"Malformed UWOP_ALLOC_LARGE"));
                    }

                    const U16 ScaledSize =
                        ReadUnwindSlotU16(UnwindInfo.Codes[Index + 1]);

                    StackPointer +=
                        static_cast<U64>(ScaledSize) * 8;

                    ++Index;
                }
                else if (Code.OpInfo == 1)
                {
                    if (Index + 2 >= UnwindInfo.Codes.size())
                    {
                        return Expected<X64UnwindResult>::Failure(
                            DebugError::Failure(
                                ErrorCode::SymbolFailure,
                                L"Malformed UWOP_ALLOC_LARGE"));
                    }

                    const U32 Size =
                        static_cast<U32>(
                            UnwindInfo.Codes[Index + 1].CodeOffset |
                            (UnwindInfo.Codes[Index + 1].UnwindOp << 8) |
                            (UnwindInfo.Codes[Index + 2].CodeOffset << 16) |
                            (UnwindInfo.Codes[Index + 2].UnwindOp << 24));

                    StackPointer += Size;
                    Index += 2;
                }
                else
                {
                    return Expected<X64UnwindResult>::Failure(
                        DebugError::Failure(
                            ErrorCode::NotSupported,
                            L"Unsupported UWOP_ALLOC_LARGE encoding"));
                }

                break;
            }

            case X64UnwindOp::SetFpReg:
            {
                //
                // Already accounted for by choosing FrameBase from FrameRegister.
                //
                break;
            }

            default:
                return Expected<X64UnwindResult>::Failure(
                    DebugError::Failure(
                        ErrorCode::NotSupported,
                        L"Unsupported unwind operation"));
            }
        }

        auto ReturnAddress = ReadU64FromTarget(
            Target,
            CpuId,
            StackPointer);

        if (!ReturnAddress.HasValue())
        {
            return Expected<X64UnwindResult>::Failure(
                ReturnAddress.Error);
        }

        Caller.Rip = ReturnAddress.Value;
        Caller.Rsp = StackPointer + 8;

        X64UnwindResult Result{};
        Result.CallerContext = Caller;
        Result.EstablisherFrame = FrameBase;
        Result.ReturnAddressLocation = StackPointer;
        Result.ReturnAddress = ReturnAddress.Value;
        Result.IsLeafFunction = false;

        return Expected<X64UnwindResult>::Success(Result);
    }

    Expected<std::vector<StackFrame>>
        X64PeUnwindStackWalker::WalkStack(
            IDebugTarget& Target,
            U32 CpuId,
            const StackWalkOptions& Options)
    {
        auto Registers = Target.GetRegisters(CpuId);

        if (!Registers.HasValue())
        {
            return Expected<std::vector<StackFrame>>::Failure(
                Registers.Error);
        }

        std::vector<StackFrame> Frames;

        RegisterContext Context = Registers.Value;

        for (U32 Index = 0; Index < Options.MaxFrames; ++Index)
        {
            if (!IsPlausibleCodeAddress(Context.Rip))
            {
                break;
            }

            StackFrame Frame{};
            Frame.Index = Index;
            Frame.InstructionPointer = Context.Rip;
            Frame.StackPointer = Context.Rsp;
            Frame.FramePointer = Context.Rbp;
            Frame.IsCurrentFrame = (Index == 0);

            //
            // Unwind the current frame into its caller context. This may use
            // PE x64 unwind metadata, or leaf-function rules if no runtime
            // function entry exists.
            //
            auto Unwind = UnwindFrame(
                Target,
                CpuId,
                Context);

            if (Unwind.HasValue())
            {
                Frame.EstablisherFrame =
                    Unwind.Value.EstablisherFrame;

                Frame.ReturnAddressLocation =
                    Unwind.Value.ReturnAddressLocation;

                Frame.ReturnAddress =
                    Unwind.Value.ReturnAddress;

                Frame.IsLeafFunction =
                    Unwind.Value.IsLeafFunction;
            }

            Frames.push_back(Frame);

            if (!Unwind.HasValue())
            {
                break;
            }

            const U64 CallerRip =
                Unwind.Value.CallerContext.Rip;

            if (!IsPlausibleCodeAddress(CallerRip))
            {
                break;
            }

            if (CallerRip == Context.Rip)
            {
                break;
            }

            Context = Unwind.Value.CallerContext;
        }

        return Expected<std::vector<StackFrame>>::Success(
            std::move(Frames));
    }

    bool
        X64PeUnwindStackWalker::IsPlausibleCodeAddress(
            U64 Address)
    {
        if (!IsCanonicalX64Address(Address) || Address == 0)
        {
            return false;
        }

        if (ModuleManager == nullptr)
        {
            return true;
        }

        auto Module = ModuleManager->GetModuleByAddress(Address);

        if (!Module.HasValue())
        {
            return false;
        }

        auto RuntimeFunction =
            UnwindData.LookupRuntimeFunction(Module.Value, Address);

        if (RuntimeFunction.HasValue())
        {
            return true;
        }

        //
        // Optional fallback: allow exact/function-symbol code address.
        //
        return false;
    }
}