#pragma once

#include <Genode/IO/ResourceLoader.hpp>

#include <SFML/Audio/SoundBuffer.hpp>

namespace Gx
{
    class SoundBufferLoader final : public ResourceLoader<sf::SoundBuffer>
    {
    public:
        SoundBufferLoader() = default;

        [[nodiscard]] ResourcePtr<sf::SoundBuffer> LoadFromFile(const std::filesystem::path& fileName, const ResourceContext& ctx) const override;
        [[nodiscard]] ResourcePtr<sf::SoundBuffer> LoadFromMemory(void* data, std::size_t size, const ResourceContext& ctx) const override;
        [[nodiscard]] ResourcePtr<sf::SoundBuffer> LoadFromStream(sf::InputStream& stream, const ResourceContext& ctx) const override;
    };
}
