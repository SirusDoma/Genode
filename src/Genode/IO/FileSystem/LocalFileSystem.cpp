#include <Genode/IO/FileSystem/LocalFileSystem.hpp>

#include <SFML/System/FileInputStream.hpp>

#include <Genode/IO/FileSystem/FileInfo.hpp>
#include <Genode/Utilities/StringHelper.hpp>

#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <filesystem>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>

typedef OSStatus (*SecTranslocateIsTranslocatedURLFunc)(CFURLRef url, Boolean* isTranslocated);
typedef CFURLRef __nullable (*SecTranslocateCreateOriginalPathForURLFunc)(CFURLRef translocatedPath, CFErrorRef* __nullable error);
#endif

namespace Gx
{
    LocalFileSystem& LocalFileSystem::Instance()
    {
        static ResourcePtr<LocalFileSystem> instance;
        if (!instance)
            instance = ResourcePtr<LocalFileSystem>(new LocalFileSystem(), [] (auto ptr) { delete ptr; });

        return *instance.get();
    }

    std::filesystem::path LocalFileSystem::GetWorkingDirectory()
    {
        // Windows and Linux doesn't have to be using the actual path of the executable.
        // This because the operating system allows the user to manually specify working directory on their own.

        // On the other hand, macOS "security" modify the working directory out of developer / user will.
        // Context: https://lapcatsoftware.com/articles/app-translocation.html

        // De-translocate is not worth, workaround has been breaking multiple times since it relies on private API
        return std::filesystem::current_path();
    }

    void LocalFileSystem::SetWorkingDirectory(const std::filesystem::path& inputPath)
    {
        auto workingDir = inputPath;

#ifdef __APPLE__
        if (workingDir.has_filename() || workingDir.has_extension())
            workingDir = workingDir.parent_path();
#endif

        if (!exists(workingDir))
            return;

        std::filesystem::current_path(workingDir);
    }

    std::vector<std::filesystem::path> LocalFileSystem::GetAssetPaths()
    {
        auto paths = std::vector<std::filesystem::path>();
        for (const auto& p : m_paths)
            paths.push_back(std::filesystem::weakly_canonical(p));

        return paths;
    }

    void LocalFileSystem::AddAssetPath(const std::filesystem::path& path)
    {
        m_paths.push_back(path);
    }

    ResourcePtr<sf::InputStream> LocalFileSystem::Open(const std::filesystem::path& fileName) const
    {
        const auto fileStream = new sf::FileInputStream();
        auto stream = ResourcePtr<sf::InputStream>(fileStream, [] (auto fs) { delete fs; });
        if (fileStream->open(GetFullName(fileName)))
            return stream;

        return nullptr;
    }

    std::unique_ptr<FileInfo> LocalFileSystem::GetFileInfo(const std::filesystem::path& fileName) const
    {
        const auto fullName = GetFullName(fileName);
        const auto size = GetFileSize(fullName).value_or(0);

        return std::make_unique<FileInfo>(FileInfo(*this, fullName.string(), size));
    }

    std::optional<std::size_t> LocalFileSystem::GetFileSize(const std::filesystem::path& fileName) const
    {
        if (auto fileStream = sf::FileInputStream(); fileStream.open(GetFullName(fileName)))
            return fileStream.getSize();

        return std::nullopt;
    }

    bool LocalFileSystem::Contains(const std::filesystem::path& fileName) const
    {
        if (fileName.empty())
            return false;

        if (std::filesystem::exists(fileName))
            return true;

        return std::any_of(m_paths.begin(), m_paths.end(), [&fileName] (const std::filesystem::path& path)
        {
            const std::filesystem::path fullPath = path.string().append("/").append(fileName.string());
            return std::filesystem::exists(fullPath);
        });
    }

    std::filesystem::path LocalFileSystem::GetFileName(const std::filesystem::path& fullPath, const bool withExtension) const
    {
        if (withExtension)
            return fullPath.filename();

        return fullPath.filename().replace_extension();
    }

    std::filesystem::path LocalFileSystem::GetFullName(const std::filesystem::path& fileName, const bool withExtension) const
    {
        if (std::filesystem::exists(fileName))
        {
            if (withExtension)
                return fileName;

            return std::filesystem::path(fileName).replace_extension();
        }

        for (std::filesystem::path& dir : m_paths)
        {
            if (std::filesystem::path fullPath = dir.string().append("/").append(fileName.string()); std::filesystem::exists(fullPath))
            {
                if (withExtension)
                    return fullPath;

                return std::filesystem::path(fullPath).replace_extension();
            }
        }

        return "";
    }

    std::optional<std::size_t> LocalFileSystem::ReadFile(const std::filesystem::path& fileName, void* data, std::size_t size) const
    {
        sf::FileInputStream fs;
        if (!fs.open(GetFullName(fileName)))
            return -1;

        if (size < 0)
            size = fs.getSize().value_or(0);

        if (size <= 0)
            return std::nullopt;

        return fs.read(data, size);
    }

    std::vector<std::byte> LocalFileSystem::ReadFile(const std::filesystem::path& fileName) const
    {
        sf::FileInputStream fs;
        if (!fs.open(GetFullName(fileName)))
            return {};

        const auto size = fs.getSize().value_or(0);
        if (size == 0)
            return {};

        std::vector<std::byte> data(size);
        const auto read = fs.read(data.data(), data.size());
        if (!read.has_value())
            return {};

        data.resize(read.value());
        return data;
    }

    void LocalFileSystem::WriteFile(const std::filesystem::path& fileName, const void* data, const std::size_t size)
    {
        if (size <= 0)
            return;

        std::ofstream fs(fileName, std::ios::out | std::ios::binary);
        fs.write(static_cast<const char*>(data), size);

        fs.close();
    }

    std::vector<std::unique_ptr<FileInfo>> LocalFileSystem::Scan(const std::string& pattern, const bool recursive) const
    {
        std::vector<std::unique_ptr<FileInfo>> files;
        std::unordered_set<std::string> scanned;

        auto paths = m_paths;
        for (const auto& dir : paths)
        {
            if (!std::filesystem::exists(dir))
                continue;

            for (const auto& entry : std::filesystem::directory_iterator(dir))
            {
                if (is_directory(entry) && recursive)
                {
                    paths.push_back(entry.path());
                    continue;
                }

                if (!is_regular_file(entry))
                    continue;

                auto fileName = weakly_canonical(entry.path()).string();
                if (auto [_, inserted] = scanned.insert(fileName); !inserted)
                    continue;

                if (StringHelper::IsGlobMatch(entry.path().filename().string(), pattern, false))
                    files.push_back(std::make_unique<FileInfo>(*this, fileName, GetFileSize(fileName).value_or(0)));
            }
        }

        return files;
    }
}
