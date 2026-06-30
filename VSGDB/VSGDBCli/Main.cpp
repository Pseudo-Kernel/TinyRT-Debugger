#include <cstdio>
#include <cstdlib>

#include <VSGDBCore/GdbRemoteTarget.h>
#include <VSGDBCore/X64Paging.h>

static void PrintRegister(const char* Name, VSGDBCore::U64 Value)
{
    std::printf("%-8s = 0x%016llx\n", Name, Value);
}

static void PrintRegisters(const VSGDBCore::RegisterContext& Context)
{
    PrintRegister("RAX", Context.Rax);
    PrintRegister("RBX", Context.Rbx);
    PrintRegister("RCX", Context.Rcx);
    PrintRegister("RDX", Context.Rdx);
    PrintRegister("RSI", Context.Rsi);
    PrintRegister("RDI", Context.Rdi);
    PrintRegister("RBP", Context.Rbp);
    PrintRegister("RSP", Context.Rsp);
    PrintRegister("R8", Context.R8);
    PrintRegister("R9", Context.R9);
    PrintRegister("R10", Context.R10);
    PrintRegister("R11", Context.R11);
    PrintRegister("R12", Context.R12);
    PrintRegister("R13", Context.R13);
    PrintRegister("R14", Context.R14);
    PrintRegister("R15", Context.R15);
    PrintRegister("RIP", Context.Rip);
    PrintRegister("RFLAGS", Context.Rflags);
    PrintRegister("CS", Context.Cs);
    PrintRegister("SS", Context.Ss);
    PrintRegister("DS", Context.Ds);
    PrintRegister("ES", Context.Es);
    PrintRegister("FS", Context.Fs);
    PrintRegister("GS", Context.Gs);

    PrintRegister("FSBASE", Context.FsBase);
    PrintRegister("GSBASE", Context.GsBase);
    PrintRegister("KGSBASE", Context.KernelGsBase);

    PrintRegister("CR0", Context.Cr0);
    PrintRegister("CR2", Context.Cr2);
    PrintRegister("CR3", Context.Cr3);
    PrintRegister("CR4", Context.Cr4);
    PrintRegister("CR8", Context.Cr8);
    PrintRegister("EFER", Context.Efer);
}

static const char*
PagingLevelName(
    VSGDBCore::X64PagingLevel Level)
{
    switch (Level)
    {
    case VSGDBCore::X64PagingLevel::Pml4:
        return "PML4";
    case VSGDBCore::X64PagingLevel::Pdpt:
        return "PDPT";
    case VSGDBCore::X64PagingLevel::Pd:
        return "PD  ";
    case VSGDBCore::X64PagingLevel::Pt:
        return "PT  ";
    default:
        return "????";
    }
}

static void
PrintPageEntry(
    const VSGDBCore::X64PageEntry& Entry)
{
    std::printf(
        "%s @ 0x%016llx = 0x%016llx  %c %c%c %c NX=%u LP=%u\n",
        PagingLevelName(Entry.Level),
        Entry.EntryPhysicalAddress,
        Entry.Value,
        Entry.Present ? 'P' : '-',
        Entry.Write ? 'W' : 'R',
        Entry.User ? 'U' : 'S',
        Entry.Global ? 'G' : '-',
        Entry.NoExecute ? 1 : 0,
        Entry.LargePage ? 1 : 0);
}

static void
PrintTranslation(
    const VSGDBCore::X64AddressTranslationResult& Translation)
{
    std::printf(
        "VA  = 0x%016llx\n",
        Translation.VirtualAddress);

    std::printf(
        "CR3 = 0x%016llx\n",
        Translation.Cr3);

    std::printf(
        "IDX = PML4[%03x] PDPT[%03x] PD[%03x] PT[%03x] OFF[%03x]\n",
        Translation.Parts.Pml4Index,
        Translation.Parts.PdptIndex,
        Translation.Parts.PdIndex,
        Translation.Parts.PtIndex,
        Translation.Parts.PageOffset);

    for (const auto& Entry : Translation.Entries)
    {
        PrintPageEntry(Entry);
    }

    if (Translation.Success)
    {
        std::printf(
            "PA  = 0x%016llx\n",
            Translation.PhysicalAddress);

        std::printf(
            "PageSize = 0x%llx\n",
            Translation.PageSize);
    }
    else
    {
        std::wprintf(
            L"Translation failed: %s\n",
            Translation.FailureMessage.c_str());
    }
}


int main(int ArgumentCount, char** Arguments)
{
    VSGDBCore::U16 Port = 1234;

    if (ArgumentCount >= 2)
    {
        Port = static_cast<VSGDBCore::U16>(
            std::strtoul(Arguments[1], nullptr, 0));
    }

    VSGDBCore::GdbRemoteTarget Target;

    VSGDBCore::DebugTargetConfig Config;
    Config.Host = L"127.0.0.1";
    Config.Port = Port;

    auto Error = Target.Connect(Config);
    if (!Error.Succeeded())
    {
        std::wprintf(
            L"Connect failed: %s Code=%d Native=0x%08x\n",
            Error.Message.c_str(),
            static_cast<int>(Error.Code),
            Error.NativeCode);
        return 1;
    }

    auto Event = Target.WaitForEvent(0);
    if (Event.Succeeded())
    {
        std::wprintf(
            L"Stop: %s\n",
            Event.Value.Description.c_str());
        std::printf("Current thread: %s\n", Event.Value.RemoteThreadId.c_str());
    }

    auto Registers = Target.GetRegisters(2);
    if (!Registers.Succeeded())
    {
        std::wprintf(
            L"GetRegisters failed: %s Code=%d Native=0x%08x\n",
            Registers.Error.Message.c_str(),
            static_cast<int>(Registers.Error.Code),
            Registers.Error.NativeCode);
        return 1;
    }

    PrintRegisters(Registers.Value);

#if 0
    auto CodeBytes = Target.ReadVirtualMemory(
        0,
        Registers.Value.Rip,
        16);

    if (!CodeBytes.Succeeded())
    {
        std::wprintf(
            L"ReadVirtualMemory failed: %s Code=%d Native=0x%08x\n",
            CodeBytes.Error.Message.c_str(),
            static_cast<int>(CodeBytes.Error.Code),
            CodeBytes.Error.NativeCode);

        return 1;
    }

    std::printf("Bytes at RIP:");
    for (VSGDBCore::U8 Byte : CodeBytes.Value)
    {
        std::printf(" %02x", Byte);
    }
    std::printf("\n");


    std::printf("Single stepping...\n");

    auto StepError = Target.StepInto();
    if (!StepError.Succeeded())
    {
        std::wprintf(
            L"StepInto failed: %s Code=%d Native=0x%08x\n",
            StepError.Message.c_str(),
            static_cast<int>(StepError.Code),
            StepError.NativeCode);

        return 1;
    }

    auto StepEvent = Target.WaitForEvent(0);
    if (StepEvent.Succeeded())
    {
        std::wprintf(
            L"Step stop: %s\n",
            StepEvent.Value.Description.c_str());
    }

    auto RegistersAfterStep = Target.GetRegisters(0);
    if (!RegistersAfterStep.Succeeded())
    {
        std::wprintf(
            L"GetRegisters after step failed: %s Code=%d Native=0x%08x\n",
            RegistersAfterStep.Error.Message.c_str(),
            static_cast<int>(RegistersAfterStep.Error.Code),
            RegistersAfterStep.Error.NativeCode);

        return 1;
    }

    std::printf("RIP before = 0x%016llx\n", Registers.Value.Rip);
    std::printf("RIP after  = 0x%016llx\n", RegistersAfterStep.Value.Rip);
#endif

#if 0
    // Breakpoint test...

    auto Registers = Target.GetRegisters(0);

    std::printf("Single stepping once before breakpoint test...\n");

    auto StepError = Target.StepInto();
    if (!StepError.Succeeded())
    {
        std::wprintf(
            L"StepInto failed: %s\n",
            StepError.Message.c_str());
        return 1;
    }

    auto StepEvent = Target.WaitForEvent(0);
    if (StepEvent.Succeeded())
    {
        std::wprintf(
            L"Step stop: %s\n",
            StepEvent.Value.Description.c_str());
    }

    auto AfterStep = Target.GetRegisters(0);
    if (!AfterStep.Succeeded())
    {
        std::wprintf(
            L"GetRegisters failed: %s\n",
            AfterStep.Error.Message.c_str());
        return 1;
    }

    std::printf(
        "Breakpoint address = 0x%016llx\n",
        AfterStep.Value.Rip);

    auto Breakpoint = Target.SetBreakpoint(
        VSGDBCore::BreakpointKind::Software,
        AfterStep.Value.Rip,
        1);

    if (!Breakpoint.Succeeded())
    {
        std::wprintf(
            L"SetBreakpoint failed: %s Code=%d Native=0x%08x\n",
            Breakpoint.Error.Message.c_str(),
            static_cast<int>(Breakpoint.Error.Code),
            Breakpoint.Error.NativeCode);
        return 1;
    }

    std::printf(
        "Breakpoint ID = %llu\n",
        Breakpoint.Value);

    std::printf("Continuing to breakpoint...\n");

    auto ContinueError = Target.Continue();
    if (!ContinueError.Succeeded())
    {
        std::wprintf(
            L"Continue failed: %s Code=%d Native=0x%08x\n",
            ContinueError.Message.c_str(),
            static_cast<int>(ContinueError.Code),
            ContinueError.NativeCode);
        return 1;
    }

    auto BreakEvent = Target.WaitForEvent(0);
    if (BreakEvent.Succeeded())
    {
        std::wprintf(
            L"Breakpoint stop: %s\n",
            BreakEvent.Value.Description.c_str());
    }

    auto AtBreakpoint = Target.GetRegisters(0);
    if (!AtBreakpoint.Succeeded())
    {
        std::wprintf(
            L"GetRegisters at breakpoint failed: %s\n",
            AtBreakpoint.Error.Message.c_str());
        return 1;
    }

    std::printf(
        "RIP at breakpoint = 0x%016llx\n",
        AtBreakpoint.Value.Rip);

    auto DeleteError = Target.DeleteBreakpoint(Breakpoint.Value);
    if (!DeleteError.Succeeded())
    {
        std::wprintf(
            L"DeleteBreakpoint failed: %s Code=%d Native=0x%08x\n",
            DeleteError.Message.c_str(),
            static_cast<int>(DeleteError.Code),
            DeleteError.NativeCode);
        return 1;
    }

    std::printf("Breakpoint deleted.\n");
#endif

    auto Threads = Target.EnumerateThreads();
    if (!Threads.Succeeded())
    {
        std::wprintf(
            L"EnumerateThreads failed: %s\n",
            Threads.Error.Message.c_str());
        return 1;
    }

    for (const auto& Thread : Threads.Value)
    {
        std::wprintf(
            L"CPU%u -> %s\n",
            Thread.CpuId,
            Thread.DisplayName.c_str());

        auto RegistersPerCpu = Target.GetRegisters(Thread.CpuId);
        if (!RegistersPerCpu.Succeeded())
        {
            std::wprintf(
                L"GetRegisters failed: %s\n",
                RegistersPerCpu.Error.Message.c_str());
        }
        else
        {
            PrintRegisters(RegistersPerCpu.Value);
        }
    }

    auto PhysicalBytes = Target.ReadPhysicalMemory(
        Registers.Value.Cr3 & 0x000ffffffffff000ull,
        16);

    if (!PhysicalBytes.Succeeded())
    {
        std::wprintf(
            L"ReadPhysicalMemory failed: %s Code=%d Native=0x%08x\n",
            PhysicalBytes.Error.Message.c_str(),
            static_cast<int>(PhysicalBytes.Error.Code),
            PhysicalBytes.Error.NativeCode);

        return 1;
    }

    std::printf("Physical bytes at CR3:");
    for (VSGDBCore::U8 Byte : PhysicalBytes.Value)
    {
        std::printf(" %02x", Byte);
    }
    std::printf("\n");

    {
        VSGDBCore::X64PagingContext Paging{};
        Paging.Cr0 = Registers.Value.Cr0;
        Paging.Cr3 = Registers.Value.Cr3;
        Paging.Cr4 = Registers.Value.Cr4;
        Paging.Efer = Registers.Value.Efer;

        auto Translation = VSGDBCore::TranslateX64VirtualAddress(
            Target,
            Paging,
            Registers.Value.Rip);

        if (!Translation.Succeeded())
        {
            std::wprintf(
                L"Translate failed: %s\n",
                Translation.Error.Message.c_str());
            return 1;
        }

        PrintTranslation(Translation.Value);

        auto PhysicalBytes = Target.ReadPhysicalMemory(
            Translation.Value.PhysicalAddress,
            16);

        if (!PhysicalBytes.Succeeded())
        {
            std::wprintf(
                L"ReadPhysicalMemory failed: %s\n",
                PhysicalBytes.Error.Message.c_str());
            return 1;
        }

        auto VirtualBytes = Target.ReadVirtualMemory(
            2,
            Registers.Value.Rip,
            16);

        if (!VirtualBytes.Succeeded())
        {
            std::wprintf(
                L"ReadPhysicalMemory failed: %s\n",
                VirtualBytes.Error.Message.c_str());
            return 1;
        }

        if (PhysicalBytes.Value.size() != VirtualBytes.Value.size())
        {
            std::printf("Physical/Virtual read mismatch\n");
            return 1;
        }

        if (memcmp(
            PhysicalBytes.Value.data(),
            VirtualBytes.Value.data(),
            PhysicalBytes.Value.size()))
        {
            std::printf("Physical/Virtual read mismatch\n");
            return 1;
        }

        std::printf("Physical/Virtual read matched\n");
    }


    return 0;
}