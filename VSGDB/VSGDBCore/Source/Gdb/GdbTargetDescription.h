#pragma once

#include <string>
#include <vector>

#include "../../Include/VSGDBCore/RegisterTypes.h"
#include "../../Include/VSGDBCore/Result.h"

namespace VSGDBCore
{
    Result<RegisterDescriptorSet> ParseGdbTargetDescription(
        const std::string& XmlText);

    std::vector<std::string> ParseGdbTargetIncludeHrefs(
        const std::string& XmlText);

    const RegisterDescriptor* FindRegisterDescriptor(
        const RegisterDescriptorSet& Set,
        const std::string& Name);

    void AppendRegisterDescriptors(
        RegisterDescriptorSet& Destination,
        const RegisterDescriptorSet& Source);
}