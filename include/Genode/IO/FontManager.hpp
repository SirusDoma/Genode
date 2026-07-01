#pragma once

#include <Genode/IO/ResourceContainer.hpp>

#include <SFML/Graphics/Font.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <mutex>

namespace Gx
{
    class FontManager
    {
    public:
        using FontData = std::vector<std::byte>;

        FontManager() = default;

        [[nodiscard]] std::unique_ptr<sf::Font> Create(const std::string& nameOrPath);
        [[nodiscard]] std::unique_ptr<sf::Font> CreateDefault();

        [[nodiscard]] std::optional<std::pair<const void*, std::size_t>> GetData(const std::string& key);
        [[nodiscard]] std::optional<std::pair<const void*, std::size_t>> GetDefaultData();

        bool Store(const std::string& key, FontData bytes);

        void Clear();

    private:
        mutable std::mutex m_mutex;
        ResourceContainer<FontData> m_cache;
    };
}
