#pragma once

#include <Genode/IO/FileSystem/FileSystemController.hpp>

#include <Genode/System/Exception.hpp>

#include <vector>
#include <string>
#include <filesystem>

namespace Gx
{
    class LocalFileSystem final : public FileSystemController
    {
    public:
        [[nodiscard]] static LocalFileSystem& Instance();

        [[nodiscard]] static std::string GetApplicationDirectoryPath();

        [[nodiscard]] static std::string GetWorkingDirectory();
        static void SetWorkingDirectory(const std::filesystem::path& inputPath);

        [[nodiscard]] static std::vector<std::string> GetAssetPaths();
        static void AddAssetPath(const std::filesystem::path& path);

        [[nodiscard]] std::string GetFileName(const std::filesystem::path& fullPath, bool withExtension = true) const;
        [[nodiscard]] std::string GetFullName(const std::filesystem::path& fileName, bool withExtension = true) const;

        [[nodiscard]] ResourcePtr<sf::InputStream> Open(const std::filesystem::path& fileName) const override;

        [[nodiscard]] bool Contains(const std::filesystem::path& fileName) const override;

        [[nodiscard]] std::unique_ptr<FileInfo> GetFileInfo(const std::filesystem::path& fileName) const override;

        [[nodiscard]] std::vector<std::unique_ptr<FileInfo>> GetFileEntries() const override { throw Gx::NotSupportedException(); }

        std::optional<std::size_t> ReadFile(const std::filesystem::path& fileName, void* data, std::size_t size) const override;
        [[nodiscard]] std::vector<std::byte> ReadFile(const std::filesystem::path& fileName) const override;
        void WriteFile(const std::filesystem::path& fileName, const void* data, std::size_t size) override;

        [[nodiscard]] std::optional<std::size_t> GetFileSize(const std::filesystem::path& fileName) const override;

        [[nodiscard]] std::vector<std::unique_ptr<FileInfo>> Scan(const std::string& pattern, bool recursive) const override;

    private:
        LocalFileSystem() = default;

        inline static std::vector<std::string> m_paths;
    };
}
