#pragma once

#include <optional>
#include <string>

namespace Gx
{
    class FontResolver
    {
    public:
        [[nodiscard]] static std::optional<std::string> Resolve(const std::string& nameOrPath);
        [[nodiscard]] static std::optional<std::string> ResolveDefault();
    };
}
