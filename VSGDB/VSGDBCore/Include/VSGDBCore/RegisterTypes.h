#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace VSGDBCore
{
    struct RegisterDescriptor
    {
        std::string Name;
        U32 Number = 0;
        U32 BitSize = 0;
    };

    struct RegisterDescriptorSet
    {
        std::vector<RegisterDescriptor> Registers;
    };
}
