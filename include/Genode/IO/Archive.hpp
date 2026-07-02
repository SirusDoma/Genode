#pragma once

#include <Genode/IO/Resource.hpp>
#include <Genode/IO/FileSystem/FileSystemController.hpp>

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace Gx
{
    class Archive : public FileSystemController
    {
    public:
        Archive() = default;

        [[nodiscard]] const std::string& GetFileName() const;

        virtual bool LoadFromFile(const std::filesystem::path& fileName);

        [[nodiscard]] ResourcePtr<sf::InputStream> Open(const std::filesystem::path& fileName) const override = 0;

        [[nodiscard]] std::vector<std::unique_ptr<FileInfo>> Scan(const std::string& pattern, bool recursive) const override;
        [[nodiscard]] bool Contains(const std::filesystem::path& fileName) const override = 0;

        [[nodiscard]] std::unique_ptr<FileInfo> GetFileInfo(const std::filesystem::path& fileName) const override = 0;

        [[nodiscard]] std::vector<std::unique_ptr<FileInfo>> GetFileEntries() const override = 0;

        std::optional<std::size_t> ReadFile(const std::filesystem::path& name, void* data, std::size_t size) const override = 0;
        virtual std::optional<std::size_t> ReadFile(const FileInfo& entry, void* data) const;

        [[nodiscard]] std::optional<std::size_t> GetFileSize(const std::filesystem::path& fileName) const override = 0;

    private:
        std::string m_filename;
    };
}
