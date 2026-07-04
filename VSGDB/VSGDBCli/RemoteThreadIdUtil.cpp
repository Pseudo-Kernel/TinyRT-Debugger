#include "RemoteThreadIdUtil.h"

#include <algorithm>
#include <cctype>

static std::string
ToLowerAscii(
    std::string Text)
{
    for (char& Character : Text)
    {
        Character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(Character)));
    }

    return Text;
}

std::string
NormalizeRemoteThreadId(
    const std::string& ThreadId)
{
    std::string Id = ToLowerAscii(ThreadId);

    //
    // QEMU can report system-mode thread IDs in at least these forms:
    //
    //   p01.03
    //   03
    //   3
    //
    // For our current VM, the part after the final '.' is the vCPU/thread
    // identifier we care about.
    //
    const size_t Dot = Id.rfind('.');
    if (Dot != std::string::npos)
    {
        Id = Id.substr(Dot + 1);
    }

    //
    // Remove leading zeroes, but keep one character if the ID is all zeroes.
    //
    while (Id.size() > 1 && Id[0] == '0')
    {
        Id.erase(Id.begin());
    }

    return Id;
}

bool
AreSameRemoteThreadId(
    const std::string& Left,
    const std::string& Right)
{
    return NormalizeRemoteThreadId(Left) ==
        NormalizeRemoteThreadId(Right);
}