#include <Genode/IO/FileSystem/FileSystemController.hpp>

namespace Gx
{
    std::vector<std::unique_ptr<FileInfo>> FileSystemController::Scan(const std::string& pattern, bool recursive) const
    {
        return {};
    }

    std::vector<std::byte> FileSystemController::ReadFile(const std::string& fileName) const
    {
        const auto size = GetFileSize(fileName);
        if (!size.has_value())
            return {};

        std::vector<std::byte> data(size.value());
        const auto read = ReadFile(fileName, data.data(), data.size());
        if (!read.has_value())
            return {};

        data.resize(read.value());
        return data;
    }

    const std::string& FileSystemController::GetPrefix() const
    {
        return m_prefix;
    }

    void FileSystemController::SetPathPrefix(const std::string& prefix)
    {
        m_prefix = prefix;
    }
}