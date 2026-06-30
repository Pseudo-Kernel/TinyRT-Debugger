
#include <cstdio>
#include <set>
#include <VSGDBCore/GdbRemoteTarget.h>

#include "GdbRegisterCodec.h"
#include "GdbPacket.h"
#include "GdbTargetDescription.h"

namespace VSGDBCore
{
    static Result<int>
        GetGdbBreakpointType(
            BreakpointKind Kind)
    {
        switch (Kind)
        {
        case BreakpointKind::Software:
            return Result<int>::Success(0);

        case BreakpointKind::HardwareExecute:
            return Result<int>::Success(1);

        case BreakpointKind::HardwareWrite:
            return Result<int>::Success(2);

        case BreakpointKind::HardwareAccess:
            return Result<int>::Success(4);

        default:
            return Result<int>::Failure(
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
        if (!Error.Succeeded())
        {
            return Error;
        }

        auto Reply = Connection.ReceiveResponsePacket();
        if (!Reply.Succeeded())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.Succeeded())
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
        if (!ThreadId.Succeeded())
        {
            return ThreadId.Error;
        }

        DebugError SelectError = SelectContinueThread(ThreadId.Value);
        if (!SelectError.Succeeded())
        {
            return SelectError;
        }

        auto Reply = Connection.SendCommand("c");

        if (!Reply.Succeeded())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.Succeeded())
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
        auto Reply = Connection.SendCommand("c");

        if (!Reply.Succeeded())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.Succeeded())
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
        if (!ThreadId.Succeeded())
        {
            return ThreadId.Error;
        }

        DebugError SelectError = SelectContinueThread(ThreadId.Value);
        if (!SelectError.Succeeded())
        {
            return SelectError;
        }

        auto Reply = Connection.SendCommand("s");

        if (!Reply.Succeeded())
        {
            return Reply.Error;
        }

        auto Event = DecodeStopReply(Reply.Value);
        if (!Event.Succeeded())
        {
            return Event.Error;
        }

        LastEvent = Event.Value;
        HasLastEvent = true;

        return DebugError::Success();
    }

    Result<DebugEvent>
        GdbRemoteTarget::WaitForEvent(
            U32 TimeoutMilliseconds)
    {
        (void)TimeoutMilliseconds;

        if (HasLastEvent)
        {
            HasLastEvent = false;
            return Result<DebugEvent>::Success(LastEvent);
        }

        auto Reply = Connection.SendCommand("?");

        if (!Reply.Succeeded())
        {
            return Result<DebugEvent>::Failure(Reply.Error);
        }

        return DecodeStopReply(Reply.Value);
    }

    Result<RegisterContext>
        GdbRemoteTarget::GetRegisters(
            U32 CpuId)
    {
        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.Succeeded())
        {
            return Result<RegisterContext>::Failure(ThreadId.Error);
        }

        DebugError SelectError = SelectRegisterThread(ThreadId.Value);
        if (!SelectError.Succeeded())
        {
            return Result<RegisterContext>::Failure(SelectError);
        }

        auto Reply = Connection.SendCommand("g");

        if (!Reply.Succeeded())
        {
            return Result<RegisterContext>::Failure(Reply.Error);
        }

        auto Decoded = DecodeX64GdbRegisters(Reply.Value);
        if (!Decoded.Succeeded())
        {
            return Decoded;
        }

        RegisterContext Context = Decoded.Value;

        auto TryRead = [&](const char* Name, U64& Field)
            {
                auto Value = ReadRegisterByName(CpuId, Name);
                if (Value.Succeeded())
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

        return Result<RegisterContext>::Success(Context);
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

    Result<ByteVector>
        GdbRemoteTarget::ReadVirtualMemory(
            U32 CpuId,
            U64 Address,
            U32 Size)
    {
        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.Succeeded())
        {
            return Result<ByteVector>::Failure(ThreadId.Error);
        }

        DebugError SelectError = SelectRegisterThread(ThreadId.Value);
        if (!SelectError.Succeeded())
        {
            return Result<ByteVector>::Failure(SelectError);
        }

        if (Size == 0)
        {
            return Result<ByteVector>::Success(ByteVector{});
        }

        char Command[64]{};

        sprintf_s(
            Command,
            "m%llx,%x",
            Address,
            Size);

        auto Reply = Connection.SendCommand(Command);

        if (!Reply.Succeeded())
        {
            return Result<ByteVector>::Failure(Reply.Error);
        }

        if (!Reply.Value.empty() && Reply.Value[0] == 'E')
        {
            return Result<ByteVector>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target failed to read memory."));
        }

        return DecodeHexBytes(Reply.Value);
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

        if (!Reply.Succeeded())
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

    Result<ByteVector>
        GdbRemoteTarget::ReadPhysicalMemory(
            U64 Address,
            U32 Size)
    {
        (void)Address;
        (void)Size;

        return Result<ByteVector>::Failure(
            DebugError::Failure(
                ErrorCode::NotSupported,
                L"ReadPhysicalMemory is not implemented yet."));
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

    Result<BreakpointId>
        GdbRemoteTarget::SetBreakpoint(
            BreakpointKind Kind,
            U64 Address,
            U32 Size)
    {
        auto Type = GetGdbBreakpointType(Kind);
        if (!Type.Succeeded())
        {
            return Result<BreakpointId>::Failure(Type.Error);
        }

        if (Size == 0)
        {
            return Result<BreakpointId>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Breakpoint size is zero."));
        }

        char Command[64]{};

        sprintf_s(
            Command,
            "Z%d,%llx,%x",
            Type.Value,
            Address,
            Size);

        auto Reply = Connection.SendCommand(Command);

        if (!Reply.Succeeded())
        {
            return Result<BreakpointId>::Failure(Reply.Error);
        }

        if (Reply.Value != "OK")
        {
            return Result<BreakpointId>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target failed to set breakpoint."));
        }

        BreakpointInfo Info{};
        Info.Id = NextBreakpointId++;
        Info.Kind = Kind;
        Info.Address = Address;
        Info.Size = Size;
        Info.Enabled = true;

        Breakpoints.emplace(Info.Id, Info);

        return Result<BreakpointId>::Success(Info.Id);
    }

    DebugError
        GdbRemoteTarget::DeleteBreakpoint(
            BreakpointId Id)
    {
        auto Entry = Breakpoints.find(Id);
        if (Entry == Breakpoints.end())
        {
            return DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"Breakpoint ID does not exist.");
        }

        const BreakpointInfo Info = Entry->second;

        auto Type = GetGdbBreakpointType(Info.Kind);
        if (!Type.Succeeded())
        {
            return Type.Error;
        }

        char Command[64]{};

        sprintf_s(
            Command,
            "z%d,%llx,%x",
            Type.Value,
            Info.Address,
            Info.Size);

        auto Reply = Connection.SendCommand(Command);

        if (!Reply.Succeeded())
        {
            return Reply.Error;
        }

        if (Reply.Value != "OK")
        {
            return DebugError::Failure(
                ErrorCode::BackendFailure,
                L"GDB remote target failed to delete breakpoint.");
        }

        Breakpoints.erase(Entry);

        return DebugError::Success();
    }

    Result<DebugEvent>
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
            return Result<DebugEvent>::Success(Event);
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

            return Result<DebugEvent>::Success(Event);
        }

        if (Reply[0] == 'W' || Reply[0] == 'X')
        {
            Event.StopReason = DebugStopReason::Exited;
            return Result<DebugEvent>::Success(Event);
        }

        Event.StopReason = DebugStopReason::Unknown;
        return Result<DebugEvent>::Success(Event);
    }

    DebugError
        GdbRemoteTarget::SelectRegisterThread(
            const std::string& ThreadId)
    {
        std::string Command = "Hg";
        Command += ThreadId;

        auto Reply = Connection.SendCommand(Command);
        if (!Reply.Succeeded())
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
        if (!Reply.Succeeded())
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

    DebugError
        GdbRemoteTarget::SelectRegisterThreadForTest(
            const std::string& ThreadId)
    {
        return SelectRegisterThread(ThreadId);
    }

    Result<std::vector<DebugThreadInfo>>
        GdbRemoteTarget::EnumerateThreads()
    {
        std::vector<std::string> RemoteThreadIds;

        bool First = true;

        for (;;)
        {
            auto Reply = Connection.SendCommand(
                First ? "qfThreadInfo" : "qsThreadInfo");

            if (!Reply.Succeeded())
            {
                return Result<std::vector<DebugThreadInfo>>::Failure(
                    Reply.Error);
            }

            First = false;

            if (Reply.Value == "l")
            {
                break;
            }

            if (Reply.Value.empty())
            {
                return Result<std::vector<DebugThreadInfo>>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"GDB remote target returned an empty thread-list reply."));
            }

            if (Reply.Value[0] != 'm')
            {
                return Result<std::vector<DebugThreadInfo>>::Failure(
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

        return Result<std::vector<DebugThreadInfo>>::Success(Threads);
    }

    Result<std::string>
        GdbRemoteTarget::GetRemoteThreadIdFromCpuId(
            U32 CpuId)
    {
        if (Threads.empty())
        {
            auto ThreadResult = EnumerateThreads();
            if (!ThreadResult.Succeeded())
            {
                return Result<std::string>::Failure(ThreadResult.Error);
            }

            Threads = ThreadResult.Value;
        }

        for (const DebugThreadInfo& Thread : Threads)
        {
            if (Thread.CpuId == CpuId)
            {
                return Result<std::string>::Success(Thread.RemoteThreadId);
            }
        }

        return Result<std::string>::Failure(
            DebugError::Failure(
                ErrorCode::InvalidArgument,
                L"CPU ID does not exist in GDB thread list."));
    }

    Result<std::string>
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
            if (!Reply.Succeeded())
            {
                return Result<std::string>::Failure(Reply.Error);
            }

            if (Reply.Value.empty())
            {
                return Result<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Empty qXfer target.xml reply."));
            }

            const char Kind = Reply.Value[0];

            if (Kind != 'm' && Kind != 'l')
            {
                return Result<std::string>::Failure(
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

        return Result<std::string>::Success(Xml);
    }

    Result<U64>
        GdbRemoteTarget::ReadRegisterByName(
            U32 CpuId,
            const std::string& Name)
    {
        DebugError DescriptionError = EnsureTargetDescriptionLoaded();
        if (!DescriptionError.Succeeded())
        {
            return Result<U64>::Failure(DescriptionError);
        }

#if 0 // for test
        for (const auto& Register : RegisterDescriptors.Registers)
        {
            printf(
                "GDB register %-16s regnum=%u bits=%u\n",
                Register.Name.c_str(),
                Register.Number,
                Register.BitSize);
        }
#endif

        const RegisterDescriptor* Descriptor =
            FindRegisterDescriptor(RegisterDescriptors, Name);

        if (!Descriptor)
        {
            return Result<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::InvalidArgument,
                    L"Register was not found in target description."));
        }

        auto ThreadId = GetRemoteThreadIdFromCpuId(CpuId);
        if (!ThreadId.Succeeded())
        {
            return Result<U64>::Failure(ThreadId.Error);
        }

        DebugError SelectError = SelectRegisterThread(ThreadId.Value);
        if (!SelectError.Succeeded())
        {
            return Result<U64>::Failure(SelectError);
        }

        char Command[32]{};

        sprintf_s(
            Command,
            "p%x",
            Descriptor->Number);

        auto Reply = Connection.SendCommand(Command);
        if (!Reply.Succeeded())
        {
            return Result<U64>::Failure(Reply.Error);
        }

        if (Reply.Value.empty() || Reply.Value[0] == 'E')
        {
            return Result<U64>::Failure(
                DebugError::Failure(
                    ErrorCode::BackendFailure,
                    L"GDB remote target failed to read register."));
        }

        return DecodeGdbRegisterValueToU64(
            Reply.Value,
            Descriptor->BitSize);
    }

    Result<std::string>
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
            if (!Reply.Succeeded())
            {
                return Result<std::string>::Failure(Reply.Error);
            }

            if (Reply.Value.empty())
            {
                return Result<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Empty qXfer features reply."));
            }

            if (Reply.Value[0] == 'E')
            {
                return Result<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"qXfer features read failed."));
            }

            const char Kind = Reply.Value[0];

            if (Kind != 'm' && Kind != 'l')
            {
                return Result<std::string>::Failure(
                    DebugError::Failure(
                        ErrorCode::BackendFailure,
                        L"Invalid qXfer features reply."));
            }

            const std::string ChunkData = Reply.Value.substr(1);

            if (ChunkData.empty() && Kind == 'm')
            {
                return Result<std::string>::Failure(
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

        return Result<std::string>::Success(Xml);
    }

    Result<RegisterDescriptorSet>
        GdbRemoteTarget::ReadTargetDescriptionTree(
            const std::string& FileName,
            std::set<std::string>& VisitedFiles)
    {
        if (VisitedFiles.find(FileName) != VisitedFiles.end())
        {
            return Result<RegisterDescriptorSet>::Success(
                RegisterDescriptorSet{});
        }

        VisitedFiles.insert(FileName);

        auto Xml = ReadTargetDescriptionFile(FileName);
        if (!Xml.Succeeded())
        {
            return Result<RegisterDescriptorSet>::Failure(Xml.Error);
        }

        auto Parsed = ParseGdbTargetDescription(Xml.Value);
        if (!Parsed.Succeeded())
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

            if (!Child.Succeeded())
            {
                return Child;
            }

            AppendRegisterDescriptors(
                Combined,
                Child.Value);
        }

        return Result<RegisterDescriptorSet>::Success(Combined);
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

        if (!Description.Succeeded())
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


}