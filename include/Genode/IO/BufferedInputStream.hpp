#pragma once

#include <SFML/System/MemoryInputStream.hpp>

#include <cstddef>
#include <vector>

namespace Gx
{
    namespace priv
    {
        struct StreamBuffer
        {
            std::vector<std::byte> Data;
        };
    }

    class BufferedInputStream : priv::StreamBuffer, public sf::MemoryInputStream
    {
    public:
        BufferedInputStream(const void* data, const std::size_t length) :
            StreamBuffer{std::vector<std::byte>(static_cast<const std::byte*>(data), static_cast<const std::byte*>(data) + length)},
            sf::MemoryInputStream(Data.data(), Data.size())
        {
        }

        explicit BufferedInputStream(std::vector<std::byte>&& data) :
            StreamBuffer{std::move(data)},
            sf::MemoryInputStream(Data.data(), Data.size())
        {
        }
    };
}
