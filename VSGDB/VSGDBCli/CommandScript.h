#pragma once

#include <string>

class CommandProcessor;

bool
ExecuteCommandScript(
    CommandProcessor& Processor,
    const std::wstring& ScriptPath);
