#pragma once

#include <Genode/IO/Resource.hpp>

#include <SFML/System/InputStream.hpp>

#include <memory>
#include <vector>
#include <string>
#include <cstddef>
#include <filesystem>

namespace Gx
{
    class FileInfo;
    class FileSystemController;
    class FileSystem final
    {
        public:
            FileSystem() = delete;
            ~FileSystem() = delete;

            [[nodiscard]] static ResourcePtr<sf::InputStream> Open(const std::filesystem::path& fileName);

            [[nodiscard]] static std::vector<std::unique_ptr<FileInfo>> Scan(const std::string& pattern);

            [[nodiscard]] static bool Contains(const std::filesystem::path& fileName);
            [[nodiscard]] static std::unique_ptr<FileInfo> GetFileInfo(const std::filesystem::path& fileName);

            static std::optional<std::size_t> ReadFile(const std::filesystem::path& fileName, void* data, std::size_t size);
            [[nodiscard]] static std::vector<std::byte> ReadFile(const std::filesystem::path& fileName);
            [[nodiscard]] static std::optional<std::size_t> GetFileSize(const std::filesystem::path& fileName);

            [[nodiscard]] static bool IsMounted(const FileSystemController& fileSystem);
            static void Mount(const FileSystemController& fileSystem);
            static void Dismount(const FileSystemController& fileSystem);

        private:
            using FileSystemMap = std::vector<const FileSystemController*>;

            inline static FileSystemMap m_controllers;
    };

}
