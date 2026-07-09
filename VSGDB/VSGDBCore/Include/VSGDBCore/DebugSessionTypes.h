#pragma once

namespace VSGDBCore
{
    enum class DebugSessionState
    {
        Disconnected,
        Stopped,
        Running,
        StopPending
    };
}