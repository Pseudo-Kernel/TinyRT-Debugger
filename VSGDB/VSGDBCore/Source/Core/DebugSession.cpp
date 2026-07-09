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

    Expected<BreakpointId>
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
                return Expected<BreakpointId>::Failure(StateError);
            }
        }

        return Target->SetBreakpoint(
            Kind,
            Address,
            Size);
    }

    DebugError
        DebugSession::DeleteBreakpoint(
            BreakpointId Id)
    {
        return DebugError::Failure(
            ErrorCode::InternalError, 
            L"Not implemented");
    }

    DebugError
        DebugSession::DeleteBreakpointByAddress(
            BreakpointKind Kind, 
            U64 Address, 
            U32 Size)
    {
        return DebugError::Failure(
            ErrorCode::InternalError,
            L"Not implemented");
    }

    DebugError
        DebugSession::RemoveBreakpointFromTarget(
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
        //return Target->RemoveBreakpointFromTarget(Id);

        return DebugError::Failure(
            ErrorCode::InternalError,
            L"Not implemented");
    }

    DebugError
        DebugSession::InsertBreakpointIntoTarget(
            BreakpointId Id)
    {
        return DebugError::Failure(
            ErrorCode::InternalError,
            L"Not implemented");
    }

    DebugError
        DebugSession::DeleteAllBreakpoints()
    {
        return DebugError::Failure(
            ErrorCode::InternalError,
            L"Not implemented");
    }

}
