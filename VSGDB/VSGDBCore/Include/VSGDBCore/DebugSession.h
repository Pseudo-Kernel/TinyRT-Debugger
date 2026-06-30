#pragma once

#include <memory>

#include "IDebugSession.h"

namespace VSGDBCore
{
    std::unique_ptr<IDebugSession> CreateDebugSession();
}