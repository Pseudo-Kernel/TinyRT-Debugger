#pragma once

#include <VSGDBCore/Result.h>
#include <VSGDBCore/Types.h>

#include <string>

VSGDBCore::Expected<VSGDBCore::U64>
ReadPeSizeOfImage(
    const std::wstring& ImagePath);
