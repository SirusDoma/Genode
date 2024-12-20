#pragma once

#include <Genode/IO/ResourceLoader.hpp>
#include <string>

namespace Gx
{
    class Font;
    class FontLoader : public ResourceLoader<Font>
    {
    public:
        FontLoader() = default;

        [[nodiscard]] bool IsStreaming() const override;
        void UseSmooth(bool smooth);

        [[nodiscard]] ResourcePtr<Font> LoadFromFile(const std::string& fileName, const ResourceContext& ctx) const override;
        ResourcePtr<Font> LoadFromMemory(void* data, std::size_t size, const ResourceContext& ctx) const override;
        ResourcePtr<Font> LoadFromStream(sf::InputStream& stream, const ResourceContext& ctx) const override;

    private:
        bool m_smooth = true;
    };
}
