#include "DebugSession.h"

#include <VSGDBCore/Components.h>

namespace VSGDBCore
{
    DebugSession::DebugSession()
        : Target(CreateGdbRemoteTarget())
    {
    }

    DebugSessionState
        DebugSession::GetState() const
    {
        std::lock_guard<std::mutex> Guard(Lock);
        return State;
    }

    DebugError
        DebugSession::RequireStoppedLocked() const
    {
        if (State == DebugSessionState::Stopped)
        {
            return DebugError::Success();
        }

        return DebugError::Failure(
            ErrorCode::InvalidState,
            L"Target is not stopped.");
    }

    DebugError
        DebugSession::RequireCanWaitForEventLocked() const
    {
        if (State == DebugSessionState::Running ||
            State == DebugSessionState::StopPending)
        {
            return DebugError::Success();
        }

        return DebugError::Failure(
            ErrorCode::InvalidState,
            L"No target execution event is pending.");
    }

    DebugError
        DebugSession::Connect(
            const DebugSessionConnectOptions& Options)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            if (State != DebugSessionState::Disconnected)
            {
                return DebugError::Failure(
                    ErrorCode::InvalidState,
                    L"Debug session is already connected.");
            }
        }

        DebugTargetConfig Config{};
        Config.Host = Options.Host;
        Config.Port = Options.Port;

        DebugError Error = Target->Connect(Config);
        if (!Error.IsSuccess())
        {
            return Error;
        }

        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
        }

        return DebugError::Success();
    }

    DebugError
        DebugSession::Disconnect()
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            if (State == DebugSessionState::Disconnected)
            {
                return DebugError::Success();
            }

            State = DebugSessionState::Disconnected;
        }

        return Target->Disconnect();
    }

    DebugError
        DebugSession::Continue(
            U32 CpuId)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return StateError;
            }

            State = DebugSessionState::Running;
        }

        auto SteppedOver =
            StepOverCurrentSoftwareBreakpointIfNeeded(CpuId);

        if (!SteppedOver.HasValue())
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
            return SteppedOver.Error;
        }

        DebugError Error = Target->Continue(CpuId);
        if (!Error.IsSuccess())
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
            return Error;
        }

        return DebugError::Success();
    }

    DebugError
        DebugSession::ContinueAll()
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return StateError;
            }

            State = DebugSessionState::Running;
        }

        DebugError Error = Target->ContinueAll();
        if (!Error.IsSuccess())
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
            return Error;
        }

        return DebugError::Success();
    }

    DebugError
        DebugSession::StepInto(
            U32 CpuId)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return StateError;
            }

            State = DebugSessionState::Running;
        }

        auto SteppedOver =
            StepOverCurrentSoftwareBreakpointIfNeeded(CpuId);

        if (!SteppedOver.HasValue())
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
            return SteppedOver.Error;
        }

        if (SteppedOver.Value)
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
            return DebugError::Success();
        }

        DebugError Error = Target->StepInto(CpuId);
        if (!Error.IsSuccess())
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Stopped;
            return Error;
        }

        return DebugError::Success();
    }

    DebugError
        DebugSession::BreakExecution()
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            if (State != DebugSessionState::Running)
            {
                return DebugError::Failure(
                    ErrorCode::InvalidState,
                    L"Target is not running.");
            }

            State = DebugSessionState::StopPending;
        }

        DebugError Error = Target->Break();
        if (!Error.IsSuccess())
        {
            std::lock_guard<std::mutex> Guard(Lock);
            State = DebugSessionState::Running;
            return Error;
        }

        return DebugError::Success();
    }

    Expected<DebugEvent>
        DebugSession::WaitForEvent(
            U32 TimeoutMilliseconds)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireCanWaitForEventLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<DebugEvent>::Failure(StateError);
            }
        }

        auto Event = Target->WaitForEvent(TimeoutMilliseconds);
        if (!Event.HasValue())
        {
            return Event;
        }

        {
            std::lock_guard<std::mutex> Guard(Lock);

            if (Event.Value.StopReason == DebugStopReason::Exited)
            {
                State = DebugSessionState::Disconnected;
            }
            else
            {
                State = DebugSessionState::Stopped;
            }
        }

        return Event;
    }

    Expected<std::vector<DebugThreadInfo>>
        DebugSession::EnumerateThreads()
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<std::vector<DebugThreadInfo>>::Failure(
                    StateError);
            }
        }

        return Target->EnumerateThreads();
    }

    Expected<RegisterContext>
        DebugSession::GetRegisters(
            U32 CpuId)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<RegisterContext>::Failure(StateError);
            }
        }

        return Target->GetRegisters(CpuId);
    }

    Expected<ByteVector>
        DebugSession::ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<ByteVector>::Failure(StateError);
            }
        }

        return Target->ReadVirtualMemory(
            CpuId,
            Address,
            Size);
    }

    Expected<ByteVector>
        DebugSession::ReadPhysicalMemory(
            U64 Address,
            U32 Size)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<ByteVector>::Failure(StateError);
            }
        }

        return Target->ReadPhysicalMemory(
            Address,
            Size);
    }




    Expected<DebugEvent>
        DebugSession::GetLastEvent() const
    {
        //return Target->GetLastEvent();
        return Expected<DebugEvent>::Failure(
            DebugError::Failure(
                ErrorCode::InternalError, 
                L"Not implemented"));
    }

    Expected<BreakpointInfo>
        DebugSession::SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<BreakpointInfo>::Failure(StateError);
            }
        }

        auto Id = Target->SetBreakpoint(
            Kind,
            Address,
            Size);

        if (!Id.HasValue())
        {
            return Expected<BreakpointInfo>::Failure(Id.Error);
        }

        auto Breakpoints = Target->EnumerateBreakpoints();
        if (!Breakpoints.HasValue())
        {
            return Expected<BreakpointInfo>::Failure(Breakpoints.Error);
        }

        for (const BreakpointInfo& Breakpoint : Breakpoints.Value)
        {
            if (Breakpoint.Id == Id.Value.Id)
            {
                return Expected<BreakpointInfo>::Success(Breakpoint);
            }
        }

        return Expected<BreakpointInfo>::Failure(
            DebugError::Failure(
                ErrorCode::InternalError,
                L"Breakpoint was inserted but could not be enumerated."));
    }

    DebugError
        DebugSession::DeleteBreakpoint(
            BreakpointId Id)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return StateError;
            }
        }

        return Target->DeleteBreakpoint(Id);
    }

    DebugError
        DebugSession::DeleteAllBreakpoints()
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return StateError;
            }
        }

        return Target->DeleteAllBreakpoints();
    }

    DebugError
        DebugSession::DeleteBreakpointByAddress(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return StateError;
            }
        }

        auto Breakpoints = Target->EnumerateBreakpoints();
        if (!Breakpoints.HasValue())
        {
            return Breakpoints.Error;
        }

        for (const BreakpointInfo& Breakpoint : Breakpoints.Value)
        {
            if (!Breakpoint.Enabled)
            {
                continue;
            }

            if (Breakpoint.Kind == Kind &&
                Breakpoint.Address == Address &&
                Breakpoint.Size == Size)
            {
                return Target->DeleteBreakpoint(Breakpoint.Id);
            }
        }

        return DebugError::Failure(
            ErrorCode::BreakpointFailure,
            L"Breakpoint was not found at the specified address.");
    }

    Expected<std::vector<BreakpointInfo>>
        DebugSession::EnumerateBreakpoints() const
    {
        {
            std::lock_guard<std::mutex> Guard(Lock);

            DebugError StateError = RequireStoppedLocked();
            if (!StateError.IsSuccess())
            {
                return Expected<std::vector<BreakpointInfo>>::Failure(
                    StateError);
            }
        }

        return Target->EnumerateBreakpoints();
    }

#if 0
    DebugError
        DebugSession::DisableBreakpointInTarget(
            BreakpointId Id)
    {
        //{
        //    std::lock_guard<std::mutex> Guard(Lock);
        //
        //    DebugError StateError = RequireStoppedLocked();
        //    if (!StateError.IsSuccess())
        //    {
        //        return StateError;
        //    }
        //}
        //
        //return Target->DisableBreakpointInTarget(Id);

        return DebugError::Failure(
            ErrorCode::InternalError,
            L"Not implemented");
    }

    DebugError
        DebugSession::EnableBreakpointInTarget(
            BreakpointId Id)
    {
        return DebugError::Failure(
            ErrorCode::InternalError,
            L"Not implemented");
    }
#endif



    Expected<bool>
        DebugSession::StepOverCurrentSoftwareBreakpointIfNeeded(
            U32 CpuId)
    {
        auto Registers = Target->GetRegisters(CpuId);
        if (!Registers.HasValue())
        {
            return Expected<bool>::Failure(Registers.Error);
        }

        const U64 Rip = Registers.Value.Rip;

        auto Breakpoints = Target->EnumerateBreakpoints();
        if (!Breakpoints.HasValue())
        {
            return Expected<bool>::Failure(Breakpoints.Error);
        }

        std::vector<BreakpointId> DisabledBreakpointIds;

        for (const BreakpointInfo& Breakpoint : Breakpoints.Value)
        {
            if (!Breakpoint.Enabled ||
                Breakpoint.Kind != BreakpointKind::Software ||
                Breakpoint.Address != Rip ||
                Breakpoint.Size != 1)
            {
                continue;
            }

            DisabledBreakpointIds.push_back(Breakpoint.Id);
        }

        if (DisabledBreakpointIds.empty())
        {
            return Expected<bool>::Success(false);
        }

        for (BreakpointId Id : DisabledBreakpointIds)
        {
            DebugError Error =
                Target->DisableBreakpointInTarget(Id);

            if (!Error.IsSuccess())
            {
                return Expected<bool>::Failure(Error);
            }
        }

        DebugError StepError =
            Target->StepInto(CpuId);

        DebugError FirstEnableError =
            DebugError::Success();

        for (BreakpointId Id : DisabledBreakpointIds)
        {
            DebugError Error =
                Target->EnableBreakpointInTarget(Id);

            if (!Error.IsSuccess() &&
                FirstEnableError.IsSuccess())
            {
                FirstEnableError = Error;
            }
        }

        if (!StepError.IsSuccess())
        {
            return Expected<bool>::Failure(StepError);
        }

        if (!FirstEnableError.IsSuccess())
        {
            return Expected<bool>::Failure(FirstEnableError);
        }

        return Expected<bool>::Success(true);
    }

}
