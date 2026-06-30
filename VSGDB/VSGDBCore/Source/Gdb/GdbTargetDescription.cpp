#include "GdbTargetDescription.h"

#include <cstdlib>

namespace VSGDBCore
{
    static std::string
        GetXmlAttribute(
            const std::string& Text,
            const std::string& Name)
    {
        const std::string Prefix = Name + "=\"";

        size_t Position = Text.find(Prefix);
        if (Position == std::string::npos)
        {
            return {};
        }

        Position += Prefix.size();

        const size_t End = Text.find('"', Position);
        if (End == std::string::npos)
        {
            return {};
        }

        return Text.substr(Position, End - Position);
    }

    static std::vector<std::string>
        FindXmlElements(
            const std::string& XmlText,
            const std::string& ElementPrefix)
    {
        std::vector<std::string> Elements;

        size_t Position = 0;

        for (;;)
        {
            Position = XmlText.find(ElementPrefix, Position);
            if (Position == std::string::npos)
            {
                break;
            }

            const size_t End = XmlText.find('>', Position);
            if (End == std::string::npos)
            {
                break;
            }

            Elements.push_back(
                XmlText.substr(Position, End - Position + 1));

            Position = End + 1;
        }

        return Elements;
    }

    static std::string
        RemoveXmlComments(
            const std::string& XmlText)
    {
        std::string Result;
        Result.reserve(XmlText.size());

        size_t Position = 0;

        for (;;)
        {
            const size_t CommentStart = XmlText.find("<!--", Position);

            if (CommentStart == std::string::npos)
            {
                Result += XmlText.substr(Position);
                break;
            }

            Result += XmlText.substr(Position, CommentStart - Position);

            const size_t CommentEnd = XmlText.find("-->", CommentStart + 4);

            if (CommentEnd == std::string::npos)
            {
                //
                // Malformed trailing comment. Drop it.
                //
                break;
            }

            Position = CommentEnd + 3;
        }

        return Result;
    }

    std::vector<std::string>
        ParseGdbTargetIncludeHrefs(
            const std::string& XmlText)
    {
        const std::string CleanXml = RemoveXmlComments(XmlText);

        std::vector<std::string> Hrefs;

        auto XiIncludes = FindXmlElements(CleanXml, "<xi:include");
        auto Includes = FindXmlElements(CleanXml, "<include");

        XiIncludes.insert(
            XiIncludes.end(),
            Includes.begin(),
            Includes.end());

        for (const std::string& Element : XiIncludes)
        {
            std::string Href = GetXmlAttribute(Element, "href");

            if (!Href.empty())
            {
                Hrefs.push_back(Href);
            }
        }

        return Hrefs;
    }

    Result<RegisterDescriptorSet>
        ParseGdbTargetDescription(
            const std::string& XmlText)
    {
        const std::string CleanXml = RemoveXmlComments(XmlText);

        RegisterDescriptorSet Set{};

        auto RegisterElements = FindXmlElements(CleanXml, "<reg ");

        U32 NextImplicitRegisterNumber = 0;

        for (const std::string& Element : RegisterElements)
        {
            RegisterDescriptor Descriptor{};

            Descriptor.Name = GetXmlAttribute(Element, "name");

            const std::string BitsizeText =
                GetXmlAttribute(Element, "bitsize");

            const std::string RegnumText =
                GetXmlAttribute(Element, "regnum");

            if (Descriptor.Name.empty() || BitsizeText.empty())
            {
                continue;
            }

            Descriptor.BitSize = static_cast<U32>(
                std::strtoul(BitsizeText.c_str(), nullptr, 0));

            if (Descriptor.BitSize == 0)
            {
                continue;
            }

            if (!RegnumText.empty())
            {
                Descriptor.Number = static_cast<U32>(
                    std::strtoul(RegnumText.c_str(), nullptr, 0));

                NextImplicitRegisterNumber = Descriptor.Number + 1;
            }
            else
            {
                Descriptor.Number = NextImplicitRegisterNumber++;
            }

            Set.Registers.push_back(Descriptor);
        }

        return Result<RegisterDescriptorSet>::Success(Set);
    }

    const RegisterDescriptor*
        FindRegisterDescriptor(
            const RegisterDescriptorSet& Set,
            const std::string& Name)
    {
        for (const RegisterDescriptor& Register : Set.Registers)
        {
            if (Register.Name == Name)
            {
                return &Register;
            }
        }

        return nullptr;
    }

    void
        AppendRegisterDescriptors(
            RegisterDescriptorSet& Destination,
            const RegisterDescriptorSet& Source)
    {
        Destination.Registers.insert(
            Destination.Registers.end(),
            Source.Registers.begin(),
            Source.Registers.end());
    }
}