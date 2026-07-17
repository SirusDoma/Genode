#include <Genode/IO/Impl/Win32/FontResolver.hpp>

#include <Genode/IO/FileSystem.hpp>

#include <algorithm>
#include <vector>

#define NOMINMAX
#include <windows.h>
#include <wingdi.h>

namespace Gx
{
    std::optional<std::string> FontResolver::Resolve(const std::string& nameOrPath)
    {
        if (FileSystem::Contains(nameOrPath))
            return nameOrPath;

        std::vector<std::string> candidateNames;
        if (nameOrPath.find_last_of('.') != std::string::npos)
        {
            candidateNames.push_back(nameOrPath);
        }
        else
        {
            static const char* exts[] = {".ttf", ".otf", ".ttc", ".dfont"};
            for (const char* ext : exts)
                candidateNames.emplace_back(nameOrPath + std::string(ext));
        }

        for (const auto& cand : candidateNames)
        {
            if (FileSystem::Contains(cand))
                return cand;
        }

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            char valueName[512];
            BYTE data[1024];
            DWORD valueIndex = 0;
            DWORD valueNameSize, dataSize, type;
            while (true)
            {
                valueNameSize = sizeof(valueName);
                dataSize = sizeof(data);
                const auto status = RegEnumValueA(hKey, valueIndex++, valueName, &valueNameSize, nullptr, &type, data, &dataSize);
                if (status != ERROR_SUCCESS) break;
                if (type != REG_SZ) continue;
                std::string entryName(valueName, valueNameSize);
                std::string fileName(reinterpret_cast<char*>(data));

                const bool matchesFamily = entryName.find(nameOrPath) != std::string::npos;
                const bool matchesFile = std::find(candidateNames.begin(), candidateNames.end(), fileName) != candidateNames.end();
                if (!matchesFamily && !matchesFile)
                    continue;

                std::string fullPath = fileName;
                if (fullPath.find(':') == std::string::npos && fullPath.rfind("\\\\", 0) != 0)
                {
                    char winDir[MAX_PATH] = {0};
                    if (GetWindowsDirectoryA(winDir, MAX_PATH))
                        fullPath = std::string(winDir) + "\\\\Fonts\\\\" + fileName;
                }

                if (FileSystem::Contains(fullPath))
                {
                    RegCloseKey(hKey);
                    return fullPath;
                }
            }
            RegCloseKey(hKey);
        }
        return std::nullopt;
    }

    std::optional<std::string> FontResolver::ResolveDefault()
    {
        std::string faceName;
        NONCLIENTMETRICSW ncm{};
        ncm.cbSize = sizeof(NONCLIENTMETRICSW);
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
        {
            wchar_t const* wname = ncm.lfMessageFont.lfFaceName;
            int len = WideCharToMultiByte(CP_UTF8, 0, wname, -1, nullptr, 0, nullptr, nullptr);
            if (len > 1)
            {
                faceName.resize(static_cast<size_t>(len - 1));
                WideCharToMultiByte(CP_UTF8, 0, wname, -1, faceName.data(), len, nullptr, nullptr);
            }
        }
        if (faceName.empty())
        {
            LOGFONTW lf{};
            if (SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
            {
                int len = WideCharToMultiByte(CP_UTF8, 0, lf.lfFaceName, -1, nullptr, 0, nullptr, nullptr);
                if (len > 1)
                {
                    faceName.resize(static_cast<size_t>(len - 1));
                    WideCharToMultiByte(CP_UTF8, 0, lf.lfFaceName, -1, faceName.data(), len, nullptr, nullptr);
                }
            }
        }

        if (!faceName.empty())
        {
            if (auto p = Resolve(faceName))
                return p;
        }

        if (auto p = Resolve("Arial"))
            return p;

        if (auto p = Resolve("Segoe UI"))
            return p;

        return std::nullopt;
    }
}
