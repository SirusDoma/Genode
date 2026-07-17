#include <Genode/IO/FontManager.hpp>
#include <Genode/IO/FontResolver.hpp>
#include <Genode/IO/FileSystem.hpp>
#include <Genode/IO/Archive.hpp>

#include <SFML/System/MemoryInputStream.hpp>

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string_view>

namespace Gx
{
    namespace
    {
        std::string NormalizeKey(const std::string_view nameOrPath)
        {
            std::string key{nameOrPath};
            const std::filesystem::path p{key};
            if (p.has_filename())
                key = p.filename().string();

            std::transform(key.begin(), key.end(), key.begin(), [](const unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return key;
        }
    }

    std::unique_ptr<sf::Font> FontManager::Create(const std::string& nameOrPath)
    {
        const auto key = NormalizeKey(nameOrPath);

        {
            std::scoped_lock lock(m_mutex);
            if (const auto* bytes = m_cache.Find(key))
            {
                sf::MemoryInputStream stream(bytes->data(), bytes->size());
                return std::make_unique<sf::Font>(stream);
            }
        }

        const auto path = FontResolver::Resolve(nameOrPath);
        if (!path)
            return nullptr;

        auto bytes = FileSystem::ReadFile(*path);
        if (bytes.empty())
            return nullptr;

        std::scoped_lock lock(m_mutex);
        const auto& ref = m_cache.Store(key, std::make_unique<FontData>(std::move(bytes)));
        sf::MemoryInputStream stream(ref.data(), ref.size());
        return std::make_unique<sf::Font>(stream);
    }

    std::unique_ptr<sf::Font> FontManager::CreateDefault()
    {
        const auto path = FontResolver::ResolveDefault();
        if (!path)
            return nullptr;

        return Create(*path);
    }

    std::optional<std::pair<const void*, std::size_t>> FontManager::GetData(const std::string& key)
    {
        const auto font = Create(key);
        if (!font)
            return std::nullopt;

        const auto norm = NormalizeKey(key);
        std::scoped_lock lock(m_mutex);
        if (const auto* bytes = m_cache.Find(norm))
            return std::pair<const void*, std::size_t>{bytes->data(), bytes->size()};

        return std::nullopt;
    }

    std::optional<std::pair<const void*, std::size_t>> FontManager::GetDefaultData()
    {
        const auto path = FontResolver::ResolveDefault();
        if (!path)
            return std::nullopt;

        const std::string key = NormalizeKey(std::filesystem::path(*path).filename().string());

        if (auto cached = GetData(key))
            return cached;

        const auto font = Create(*path);
        if (!font)
            return std::nullopt;

        return GetData(key);
    }

    bool FontManager::Store(const std::string& key, FontData bytes)
    {
        if (bytes.empty())
            return false;

        const auto norm = NormalizeKey(key);
        std::scoped_lock lock(m_mutex);
        if (m_cache.Find(norm))
            return false;

        m_cache.Store(norm, std::make_unique<FontData>(std::move(bytes)));
        return true;
    }

    void FontManager::Clear()
    {
        std::scoped_lock lock(m_mutex);
        m_cache.Clear();
    }
}
