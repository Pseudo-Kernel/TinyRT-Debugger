
#include <cstdio>
#include <set>
#include <VSGDBCore/GdbRemoteTarget.h>

#include "GdbRegisterCodec.h"
#include "GdbPacket.h"
#include "GdbTargetDescription.h"

namespace VSGDBCore
{
    static Expected<int>
        GetGdbBreakpointType(
            BreakpointKind Kind)
    {
        switch (Kind)
        {
        case BreakpointKind::Software:
            return Expected<int>::Success(0);

        case BreakpointKind::HardwareExecute:
            return Expected<int>::Success(1);

        case BreakpointKind::HardwareWrite:
            return Expected<int>::Success(2);

        case BreakpointKind::HardwareAccess:
            return Expected<int>::Success(4);

        default:
            return Expected<int>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Invalid breakpoint kind."));
        }
    }

    static std::string
        FindStopReplyField(
            const std::string& Reply,
            const std::string& FieldName)
    {
        const std::string Prefix = FieldName + ":";

        size_t Position = Reply.find(Prefix);
        if (Position == std::string::npos)
        {
            return {};
        }

        Position += Prefix.size();

        const size_t End = Reply.find(';', Position);
        if (End == std::string::npos)
        {
            return Reply.substr(Position);
        }

        return Reply.substr(Position, End - Position);
    }

    static std::vector<std::string>
        SplitCommaSeparatedThreadIds(
            const std::string& Text)
    {
        std::vector<std::string> Items;

        size_t Start = 0;

        while (Start < Text.size())
        {
            const size_t End = Text.find(',', Start);

            if (End == std::string::npos)
            {
                if (Start < Text.size())
                {
                    Items.push_back(Text.substr(Start));
                }

                break;
            }

            if (End > Start)
            {
                Items.push_back(Text.substr(Start, End - Start));
            }

            Start = End + 1;
        }

        return Items;
    }

    DebugError
        GdbRemoteTarget::Connect(
            const DebugTargetConfig& TargetConfig)
    {
        Config = TargetConfig;

        return Connection.Connect(
            TargetConfig.Host,
            TargetConfig.Port);
    }

    DebugError
        GdbRemoteTarget::Disconnect()
    {
        return Connection.Disconnect();
    }

    DebugError
        GdbRemoteTarget::Break()
    {
        DebugError Error = Connection.SendInterrupt();
        if (!Error.IsSuccess())
        {
            return Error;
        }

        auto Reply = Connection.ReceiveResponsePacket();
        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.HasValue())
        {
            return Event.Error;
        }

        LastEvent = Event.Value;
        HasLastEvent = true;

        return DebugError::Success();
    }

    DebugError
        GdbRemoteTarget::Continue(
            U32 CpuId)
    {
        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.HasValue())
        {
            return ThreadId.Error;
        }

        DebugError SelectError = SelectContinueThread(ThreadId.Value);
        if (!SelectError.IsSuccess())
        {
            return SelectError;
        }

        auto Reply = Connection.SendCommand("c");

        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.HasValue())
        {
            return Event.Error;
        }

        LastEvent = Event.Value;
        HasLastEvent = true;

        return DebugError::Success();
    }

    DebugError
        GdbRemoteTarget::ContinueAll()
    {
        DebugError SelectError = SelectContinueThread("-1");
        if (!SelectError.IsSuccess())
        {
            return SelectError;
        }

        auto Reply = Connection.SendCommand("c");

        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.HasValue())
        {
            return Event.Error;
        }

        LastEvent = Event.Value;
        HasLastEvent = true;

        return DebugError::Success();
    }

    DebugError
        GdbRemoteTarget::StepInto(
            U32 CpuId)
    {
        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.HasValue())
        {
            return ThreadId.Error;
        }

        DebugError SelectError = SelectContinueThread(ThreadId.Value);
        if (!SelectError.IsSuccess())
        {
            return SelectError;
        }

        auto Reply = Connection.SendCommand("s");

        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.HasValue())
        {
            return Event.Error;
        }

        LastEvent = Event.Value;
        HasLastEvent = true;

        return DebugError::Success();
    }

    Expected<DebugEvent>
        GdbRemoteTarget::WaitForEvent(
            U32 TimeoutMilliseconds)
    {
        (void)TimeoutMilliseconds;

        if (HasLastEvent)
        {
            HasLastEvent = false;
            return Expected<DebugEvent>::Success(LastEvent);
        }

        auto Reply = Connection.SendCommand("?");

        if (!Reply.HasValue())
        {
            return Expected<DebugEvent>::Failure(Reply.Error);
        }

        return DecodeStopReply(Reply.Value);
    }

    Expected<RegisterContext>
        GdbRemoteTarget::GetRegisters(
            U32 CpuId)
    {
        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.HasValue())
        {
            return Expected<RegisterContext>::Failure(ThreadId.Error);
        }

        DebugError SelectError = SelectRegisterThread(ThreadId.Value);
        if (!SelectError.IsSuccess())
        {
            return Expected<RegisterContext>::Failure(SelectError);
        }

        auto Reply = Connection.SendCommand("g");

        if (!Reply.HasValue())
        {
            return Expected<RegisterContext>::Failure(Reply.Error);
        }

        auto Decoded = DecodeX64GdbRegisters(Reply.Value);
        if (!Decoded.HasValue())
        {
            return Decoded;
        }

        RegisterContext Context = Decoded.Value;

        auto TryRead = [&](const char* Name, U64& Field)
            {
                auto Value = ReadRegisterByName(CpuId, Name);
                if (Value.HasValue())
                {
                    Field = Value.Value;
                }
            };

        TryRead("fs_base", Context.FsBase);
        TryRead("gs_base", Context.GsBase);
        TryRead("k_gs_base", Context.KernelGsBase);

        TryRead("cr0", Context.Cr0);
        TryRead("cr2", Context.Cr2);
        TryRead("cr3", Context.Cr3);
        TryRead("cr4", Context.Cr4);
        TryRead("cr8", Context.Cr8);

        TryRead("efer", Context.Efer);

        return Expected<RegisterContext>::Success(Context);
    }

    DebugError
        GdbRemoteTarget::SetRegister(
            U32 CpuId,
            const std::wstring& Name,
            U64 Value)
    {
        (void)CpuId;
        (void)Name;
        (void)Value;

        return DebugError::Failure(
            ErrorCode::NotSupported,
            L"SetRegister is not implemented yet.");
    }

    Expected<ByteVector>
        GdbRemoteTarget::ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size)
    {
        if (Size == 0)
        {
            return Expected<ByteVector>::Success(ByteVector{});
        }

        constexpr U32 MaxGdbMemoryReadSize = 0x800;

        ByteVector Result;
        Result.reserve(Size);

        U64 CurrentAddress = Address;
        U32 Remaining = Size;

        while (Remaining != 0)
        {
            const U32 CurrentSize =
                std::min<U32>(Remaining, MaxGdbMemoryReadSize);

            auto Chunk = ReadVirtualMemorySingle(
                CpuId,
                CurrentAddress,
                CurrentSize);

            if (!Chunk.HasValue())
            {
                return Chunk;
            }

            if (Chunk.Value.size() != CurrentSize)
            {
                return Expected<ByteVector>::Failure(
                    DebugError::Failure(
                        ErrorCode::MemoryReadFailure,
                        L"GDB remote target returned a short memory read."));
            }

            Result.insert(
                Result.end(),
                Chunk.Value.begin(),
                Chunk.Value.end());

            CurrentAddress += CurrentSize;
            Remaining -= CurrentSize;
        }

        return Expected<ByteVector>::Success(std::move(Result));
    }

    Expected<ByteVector>
        GdbRemoteTarget::ReadVirtualMemorySingle(
            U32 CpuId,
            U64 Address,
            U32 Size)
    {
        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.HasValue())
        {
            return Expected<ByteVector>::Failure(ThreadId.Error);
        }

        DebugError SelectError = SelectRegisterThread(ThreadId.Value);
        if (!SelectError.IsSuccess())
        {
            return Expected<ByteVector>::Failure(SelectError);
        }

        return ReadMemoryUsingGdbM(
            Address,
            Size);
    }

    DebugError
        GdbRemoteTarget::WriteVirtualMemory(
            U32 CpuId,
            U64 Address,
            const ByteVector& Bytes)
    {
        (void)CpuId;

        if (Bytes.empty())
        {
            return DebugError::Success();
        }

        const std::string HexBytes = EncodeHexBytes(Bytes);

        char Prefix[64]{};

        sprintf_s(
            Prefix,
            "M%llx,%x:",
            Address,
            static_cast<unsigned>(Bytes.size()));

        std::string Command = Prefix;
        Command += HexBytes;

        auto Reply = Connection.SendCommand(Command);

        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        if (Reply.Value == "OK")
        {
            return DebugError::Success();
        }

        return DebugError::Failure(
            ErrorCode::BackendFailure,
            L"GDB remote target failed to write memory.");
    }

    Expected<ByteVector>
        GdbRemoteTarget::ReadPhysicalMemory(
            U64 Address,
            U32 Size)
    {
        if (Size == 0)
        {
            return Expected<ByteVector>::Success(ByteVector{});
        }

        DebugError EnableError = SetQemuPhysicalMemoryMode(true);
        if (!EnableError.IsSuccess())
        {
            return Expected<ByteVector>::Failure(EnableError);
        }

        auto Bytes = ReadMemoryUsingGdbM(
            Address,
            Size);

        DebugError DisableError = SetQemuPhysicalMemoryMode(false);

        if (!DisableError.IsSuccess() && Bytes.HasValue())
        {
            return Expected<ByteVector>::Failure(DisableError);
        }

        return Bytes;
    }

    DebugError
        GdbRemoteTarget::WritePhysicalMemory(
            U64 Address,
            const ByteVector& Bytes)
    {
        (void)Address;
        (void)Bytes;

        return DebugError::Failure(
            ErrorCode::NotSupported,
            L"WritePhysicalMemory is not implemented yet.");
    }

    Expected<BreakpointInfo>
        GdbRemoteTarget::SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        DebugError Error = InsertRemoteBreakpoint(
            Kind,
            Address,
            Size);

        if (!Error.IsSuccess())
        {
            return Expected<BreakpointInfo>::Failure(Error);
        }

        const BreakpointId Id = NextBreakpointId++;

        BreakpointInfo Info{};
        Info.Id = Id;
        Info.Kind = Kind;
        Info.Address = Address;
        Info.Size = Size;
        Info.Enabled = true;
        Info.Inserted = true;

        Breakpoints.emplace(Id, Info);

        return Expected<BreakpointInfo>::Success(Info);
    }

    DebugError
        GdbRemoteTarget::DeleteBreakpoint(
            BreakpointId Id)
    {
        auto It = Breakpoints.find(Id);
        if (It == Breakpoints.end())
        {
            return DebugError::Failure(
                ErrorCode::BreakpointFailure,
                L"Breakpoint was not found.");
        }

        BreakpointInfo& Breakpoint = It->second;

        if (!Breakpoint.Enabled)
        {
            return DebugError::Failure(
                ErrorCode::InvalidState,
                L"Breakpoint is already disabled.");
        }

        if (Breakpoint.Inserted)
        {
            DebugError Error = DeleteRemoteBreakpoint(
                Breakpoint.Kind,
                Breakpoint.Address,
                Breakpoint.Size);

            if (!Error.IsSuccess())
            {
                return Error;
            }

            Breakpoint.Inserted = false;
        }

        Breakpoint.Enabled = false;
        return DebugError::Success();
    }

    Expected<std::vector<BreakpointInfo>>
        GdbRemoteTarget::EnumerateBreakpoints() const
    {
        std::vector<BreakpointInfo> Result;

        Result.reserve(Breakpoints.size());

        for (const auto& Pair : Breakpoints)
        {
            const BreakpointInfo& Breakpoint = Pair.second;

            if (!Breakpoint.Enabled)
            {
                continue;
            }

            Result.push_back(Breakpoint);
        }

        return Expected<std::vector<BreakpointInfo>>::Success(
            std::move(Result));
    }

    DebugError
        GdbRemoteTarget::DeleteBreakpointByAddress(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        for (auto& Pair : Breakpoints)
        {
            BreakpointInfo& Breakpoint = Pair.second;

            if (!Breakpoint.Enabled)
            {
                continue;
            }

            if (Breakpoint.Kind != Kind ||
                Breakpoint.Address != Address ||
                Breakpoint.Size != Size)
            {
                continue;
            }

            return DeleteBreakpoint(Breakpoint.Id);
        }

        //
        // Stale/external breakpoint path.
        //
        return DeleteRemoteBreakpoint(
            Kind,
            Address,
            Size);
    }

    DebugError
        GdbRemoteTarget::InsertRemoteBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        auto Type = GetGdbBreakpointType(Kind);
        if (!Type.HasValue())
        {
            return Type.Error;
        }

        char Packet[128]{};

        std::snprintf(
            Packet,
            sizeof(Packet),
            "Z%d,%llx,%x",
            Type.Value,
            Address,
            Size);

        auto Reply = Connection.SendCommand(Packet);
        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        if (Reply.Value == "OK")
        {
            return DebugError::Success();
        }

        if (Reply.Value.empty())
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Remote target does not support breakpoint insertion.");
        }

        return DebugError::Failure(
            ErrorCode::BreakpointFailure,
            L"Remote target rejected breakpoint insertion.");
    }

    DebugError
        GdbRemoteTarget::DeleteRemoteBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        auto Type = GetGdbBreakpointType(Kind);
        if (!Type.HasValue())
        {
            return Type.Error;
        }

        char Packet[128]{};

        std::snprintf(
            Packet,
            sizeof(Packet),
            "z%d,%llx,%x",
            Type.Value,
            Address,
            Size);

        auto Reply = Connection.SendCommand(Packet);
        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        if (Reply.Value == "OK")
        {
            return DebugError::Success();
        }

        if (Reply.Value.empty())
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"Remote target does not support breakpoint deletion.");
        }

        return DebugError::Failure(
            ErrorCode::BreakpointFailure,
            L"Remote target rejected breakpoint deletion.");
    }

    DebugError
        GdbRemoteTarget::DisableBreakpointInTarget(
            BreakpointId Id)
    {
        auto It = Breakpoints.find(Id);
        if (It == Breakpoints.end())
        {
            return DebugError::Failure(
                ErrorCode::BreakpointFailure,
                L"Breakpoint was not found.");
        }

        BreakpointInfo& Breakpoint = It->second;

        if (!Breakpoint.Enabled)
        {
            return DebugError::Failure(
                ErrorCode::InvalidState,
                L"Breakpoint is disabled.");
        }

        if (!Breakpoint.Inserted)
        {
            return DebugError::Success();
        }

        DebugError Error = DeleteRemoteBreakpoint(
            Breakpoint.Kind,
            Breakpoint.Address,
            Breakpoint.Size);

        if (!Error.IsSuccess())
        {
            return Error;
        }

        Breakpoint.Inserted = false;
        return DebugError::Success();
    }

    DebugError
        GdbRemoteTarget::EnableBreakpointInTarget(
            BreakpointId Id)
    {
        auto It = Breakpoints.find(Id);
        if (It == Breakpoints.end())
        {
            return DebugError::Failure(
                ErrorCode::BreakpointFailure,
                L"Breakpoint was not found.");
        }

        BreakpointInfo& Breakpoint = It->second;

        if (!Breakpoint.Enabled)
        {
            return DebugError::Failure(
                ErrorCode::InvalidState,
                L"Breakpoint is disabled.");
        }

        if (Breakpoint.Inserted)
        {
            return DebugError::Success();
        }

        DebugError Error = InsertRemoteBreakpoint(
            Breakpoint.Kind,
            Breakpoint.Address,
            Breakpoint.Size);

        if (!Error.IsSuccess())
        {
            return Error;
        }

        Breakpoint.Inserted = true;
        return DebugError::Success();
    }

    DebugError
        GdbRemoteTarget::DeleteAllBreakpoints()
    {
        DebugError FirstError = DebugError::Success();

        for (auto& Pair : Breakpoints)
        {
            BreakpointInfo& Breakpoint = Pair.second;

            if (!Breakpoint.Enabled)
            {
                continue;
            }

            if (Breakpoint.Inserted)
            {
                DebugError Error = DeleteRemoteBreakpoint(
                    Breakpoint.Kind,
                    Breakpoint.Address,
                    Breakpoint.Size);

                if (!Error.IsSuccess())
                {
                    if (FirstError.IsSuccess())
                    {
                        FirstError = Error;
                    }

                    continue;
                }

                Breakpoint.Inserted = false;
            }

            Breakpoint.Enabled = false;
        }

        return FirstError;
    }

    Expected<DebugEvent>
        GdbRemoteTarget::DecodeStopReply(
            const std::string& Reply) const
    {
        DebugEvent Event{};
        Event.Description = std::wstring(
            Reply.begin(),
            Reply.end());

        if (Reply.empty())
        {
            Event.StopReason = DebugStopReason::Unknown;
            return Expected<DebugEvent>::Success(Event);
        }

        if (Reply[0] == 'S' || Reply[0] == 'T')
        {
            Event.StopReason = DebugStopReason::Signal;

            if (Reply.size() >= 3)
            {
                U8 Signal = 0;
                if (DecodeHexByte(Reply[1], Reply[2], Signal))
                {
                    Event.Signal = Signal;

                    if (Signal == 5)
                    {
                        Event.StopReason = DebugStopReason::Breakpoint;
                    }
                }
            }

            Event.RemoteThreadId = FindStopReplyField(Reply, "thread");

            return Expected<DebugEvent>::Success(Event);
        }

        if (Reply[0] == 'W' || Reply[0] == 'X')
        {
            Event.StopReason = DebugStopReason::Exited;
            return Expected<DebugEvent>::Success(Event);
        }

        Event.StopReason = DebugStopReason::Unknown;
        return Expected<DebugEvent>::Success(Event);
    }

    DebugError
        GdbRemoteTarget::SelectRegisterThread(
            const std::string& ThreadId)
    {
        std::string Command = "Hg";
        Command += ThreadId;

        auto Reply = Connection.SendCommand(Command);
        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        if (Reply.Value == "OK")
        {
            return DebugError::Success();
        }

        return DebugError::Failure(
            ErrorCode::BackendFailure,
            L"GDB remote target rejected Hg thread selection.");
    }

    DebugError
        GdbRemoteTarget::SelectContinueThread(
            const std::string& ThreadId)
    {
        std::string Command = "Hc";
        Command += ThreadId;

        auto Reply = Connection.SendCommand(Command);
        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        if (Reply.Value == "OK")
        {
            return DebugError::Success();
        }

        return DebugError::Failure(
            ErrorCode::BackendFailure,
            L"GDB remote target rejected Hc thread selection.");
    }

    Expected<DebugEvent>
        GdbRemoteTarget::GetLastEvent() const
    {
        if (!HasLastEvent)
        {
            return Expected<DebugEvent>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidState,
                    L"No stop event is available."));
        }

        return Expected<DebugEvent>::Success(LastEvent);
    }

    DebugError
        GdbRemoteTarget::BreakExecution()
    {
        return Connection.SendInterrupt();
    }

    Expected<std::vector<DebugThreadInfo>>
        GdbRemoteTarget::EnumerateThreads()
    {
        std::vector<std::string> RemoteThreadIds;

        bool First = true;

        for (;;)
        {
            auto Reply = Connection.SendCommand(
                First ? "qfThreadInfo" : "qsThreadInfo");

            if (!Reply.HasValue())
            {
                return Expected<std::vector<DebugThreadInfo>>::Failure(
                    Reply.Error);
            }

            First = false;

            if (Reply.Value == "l")
            {
                break;
            }

            if (Reply.Value.empty())
            {
                return Expected<std::vector<DebugThreadInfo>>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"GDB remote target returned an empty thread-list reply."));
            }

            if (Reply.Value[0] != 'm')
            {
                return Expected<std::vector<DebugThreadInfo>>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"GDB remote target returned an invalid thread-list reply."));
            }

            auto Items = SplitCommaSeparatedThreadIds(
                Reply.Value.substr(1));

            RemoteThreadIds.insert(
                RemoteThreadIds.end(),
                Items.begin(),
                Items.end());
        }

        std::vector<DebugThreadInfo> Threads;
        Threads.reserve(RemoteThreadIds.size());

        for (size_t Index = 0; Index < RemoteThreadIds.size(); ++Index)
        {
            DebugThreadInfo Thread{};
            Thread.CpuId = static_cast<U32>(Index);
            Thread.RemoteThreadId = RemoteThreadIds[Index];

            wchar_t Name[64]{};
            swprintf_s(
                Name,
                L"vCPU%u (%S)",
                Thread.CpuId,
                Thread.RemoteThreadId.c_str());

            Thread.DisplayName = Name;

            Threads.push_back(Thread);
        }

        return Expected<std::vector<DebugThreadInfo>>::Success(Threads);
    }

    Expected<std::string>
        GdbRemoteTarget::GetRemoteThreadIdFromCpuId(
            U32 CpuId)
    {
        if (Threads.empty())
        {
            auto ThreadResult = EnumerateThreads();
            if (!ThreadResult.HasValue())
            {
                return Expected<std::string>::Failure(ThreadResult.Error);
            }

            Threads = ThreadResult.Value;
        }

        for (const DebugThreadInfo& Thread : Threads)
        {
            if (Thread.CpuId == CpuId)
            {
                return Expected<std::string>::Success(Thread.RemoteThreadId);
            }
        }

        return Expected<std::string>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"CPU ID does not exist in GDB thread list."));
    }

    Expected<std::string>
        GdbRemoteTarget::ReadTargetXml()
    {
        std::string Xml;

        U32 Offset = 0;
        constexpr U32 ChunkSize = 0x1000;

        for (;;)
        {
            char Command[96]{};

            sprintf_s(
                Command,
                "qXfer:features:read:target.xml:%x,%x",
                Offset,
                ChunkSize);

            auto Reply = Connection.SendCommand(Command);
            if (!Reply.HasValue())
            {
                return Expected<std::string>::Failure(Reply.Error);
            }

            if (Reply.Value.empty())
            {
                return Expected<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Empty qXfer target.xml reply."));
            }

            const char Kind = Reply.Value[0];

            if (Kind != 'm' && Kind != 'l')
            {
                return Expected<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Invalid qXfer target.xml reply."));
            }

            Xml += Reply.Value.substr(1);

            if (Kind == 'l')
            {
                break;
            }

            Offset += ChunkSize;
        }

        return Expected<std::string>::Success(Xml);
    }

    Expected<U64>
        GdbRemoteTarget::ReadRegisterByName(
            U32 CpuId,
            const std::string& Name)
    {
        DebugError DescriptionError = EnsureTargetDescriptionLoaded();
        if (!DescriptionError.IsSuccess())
        {
            return Expected<U64>::Failure(DescriptionError);
        }

        const RegisterDescriptor* Descriptor =
            FindRegisterDescriptor(RegisterDescriptors, Name);

        if (!Descriptor)
        {
            return Expected<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Register was not found in target description."));
        }

        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.HasValue())
        {
            return Expected<U64>::Failure(ThreadId.Error);
        }

        DebugError SelectError = SelectRegisterThread(ThreadId.Value);
        if (!SelectError.IsSuccess())
        {
            return Expected<U64>::Failure(SelectError);
        }

        char Command[32]{};

        sprintf_s(
            Command,
            "p%x",
            Descriptor->Number);

        auto Reply = Connection.SendCommand(Command);
        if (!Reply.HasValue())
        {
            return Expected<U64>::Failure(Reply.Error);
        }

        if (Reply.Value.empty() || Reply.Value[0] == 'E')
        {
            return Expected<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target failed to read register."));
        }

        return DecodeGdbRegisterValueToU64(
            Reply.Value,
            Descriptor->BitSize);
    }

    Expected<std::string>
        GdbRemoteTarget::ReadTargetDescriptionFile(
            const std::string& FileName)
    {
        std::string Xml;

        U32 Offset = 0;
        constexpr U32 ChunkSize = 0x400;

        for (;;)
        {
            char Command[256]{};

            sprintf_s(
                Command,
                "qXfer:features:read:%s:%x,%x",
                FileName.c_str(),
                Offset,
                ChunkSize);

            auto Reply = Connection.SendCommand(Command);
            if (!Reply.HasValue())
            {
                return Expected<std::string>::Failure(Reply.Error);
            }

            if (Reply.Value.empty())
            {
                return Expected<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Empty qXfer features reply."));
            }

            if (Reply.Value[0] == 'E')
            {
                return Expected<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"qXfer features read failed."));
            }

            const char Kind = Reply.Value[0];

            if (Kind != 'm' && Kind != 'l')
            {
                return Expected<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Invalid qXfer features reply."));
            }

            const std::string ChunkData = Reply.Value.substr(1);

            if (ChunkData.empty() && Kind == 'm')
            {
                return Expected<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"qXfer features returned empty intermediate chunk."));
            }

            Xml += ChunkData;

            if (Kind == 'l')
            {
                break;
            }

            Offset += static_cast<U32>(ChunkData.size());
        }

        return Expected<std::string>::Success(Xml);
    }

    Expected<RegisterDescriptorSet>
        GdbRemoteTarget::ReadTargetDescriptionTree(
            const std::string& FileName,
            std::set<std::string>& VisitedFiles)
    {
        if (VisitedFiles.find(FileName) != VisitedFiles.end())
        {
            return Expected<RegisterDescriptorSet>::Success(
                RegisterDescriptorSet{});
        }

        VisitedFiles.insert(FileName);

        auto Xml = ReadTargetDescriptionFile(FileName);
        if (!Xml.HasValue())
        {
            return Expected<RegisterDescriptorSet>::Failure(Xml.Error);
        }

        auto Parsed = ParseGdbTargetDescription(Xml.Value);
        if (!Parsed.HasValue())
        {
            return Parsed;
        }

        RegisterDescriptorSet Combined = Parsed.Value;

        const std::vector<std::string> Includes =
            ParseGdbTargetIncludeHrefs(Xml.Value);

        for (const std::string& Include : Includes)
        {
            auto Child = ReadTargetDescriptionTree(
                Include,
                VisitedFiles);

            if (!Child.HasValue())
            {
                return Child;
            }

            AppendRegisterDescriptors(
                Combined,
                Child.Value);
        }

        return Expected<RegisterDescriptorSet>::Success(Combined);
    }

    DebugError
        GdbRemoteTarget::EnsureTargetDescriptionLoaded()
    {
        if (RegisterDescriptorsLoaded)
        {
            return DebugError::Success();
        }

        std::set<std::string> VisitedFiles;

        auto Description = ReadTargetDescriptionTree(
            "target.xml",
            VisitedFiles);

        if (!Description.HasValue())
        {
            return Description.Error;
        }

        if (Description.Value.Registers.empty())
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"No registers were found in GDB target description.");
        }

        RegisterDescriptors = Description.Value;
        RegisterDescriptorsLoaded = true;

        return DebugError::Success();
    }

    DebugError
        GdbRemoteTarget::SetQemuPhysicalMemoryMode(
            bool Enabled)
    {
        auto Reply = Connection.SendCommand(
            Enabled ? "Qqemu.PhyMemMode:1" : "Qqemu.PhyMemMode:0");

        if (!Reply.HasValue())
        {
            return Reply.Error;
        }

        if (Reply.Value == "OK")
        {
            return DebugError::Success();
        }

        if (Reply.Value.empty())
        {
            return DebugError::Failure(
                ErrorCode::NotSupported,
                L"GDB remote target does not support physical memory mode.");
        }

        if (!Reply.Value.empty() && Reply.Value[0] == 'E')
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"GDB remote target rejected physical memory mode.");
        }

        return DebugError::Failure(
            ErrorCode::BackendFailure,
            L"Unexpected reply to physical memory mode command.");
    }

    Expected<ByteVector>
        GdbRemoteTarget::ReadMemoryUsingGdbM(
            U64 Address,
            U32 Size)
    {
        if (Size == 0)
        {
            return Expected<ByteVector>::Success(ByteVector{});
        }

        char Command[64]{};

        sprintf_s(
            Command,
            "m%llx,%x",
            Address,
            Size);

        auto Reply = Connection.SendCommand(Command);

        if (!Reply.HasValue())
        {
            return Expected<ByteVector>::Failure(Reply.Error);
        }

        if (!Reply.Value.empty() && Reply.Value[0] == 'E')
        {
            return Expected<ByteVector>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target failed to read memory."));
        }

        return DecodeHexBytes(Reply.Value);
    }
}