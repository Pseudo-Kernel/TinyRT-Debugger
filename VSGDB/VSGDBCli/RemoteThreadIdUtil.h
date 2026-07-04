#pragma once

#include <string>

std::string NormalizeRemoteThreadId(
    const std::string& ThreadId);

bool AreSameRemoteThreadId(
    const std::string& Left,
    const std::string& Right);
