#include <Genode/IO/Impl/macOS/FontResolver.hpp>

#include <Genode/IO/FileSystem.hpp>

#include <vector>
#include <limits.h>

#include <TargetConditionals.h>
#include <CoreText/CoreText.h>
#include <CoreFoundation/CoreFoundation.h>

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

        CFStringRef cfName = CFStringCreateWithCString(kCFAllocatorDefault, nameOrPath.c_str(), kCFStringEncodingUTF8);
        if (cfName)
        {
            CTFontDescriptorRef desc = CTFontDescriptorCreateWithNameAndSize(cfName, 12.0);
            if (desc)
            {
                CTFontRef font = CTFontCreateWithFontDescriptor(desc, 12.0, nullptr);
                if (font)
                {
                    CFURLRef url = static_cast<CFURLRef>(CTFontCopyAttribute(font, kCTFontURLAttribute));
                    if (url)
                    {
                        char path[PATH_MAX];
                        if (CFURLGetFileSystemRepresentation(url, true, reinterpret_cast<UInt8*>(path), sizeof(path)))
                        {
                            std::string full(path);
                            CFRelease(url);
                            CFRelease(font);
                            CFRelease(desc);
                            CFRelease(cfName);
                            if (FileSystem::Contains(full))
                                return full;

                            for (const auto& cand : candidateNames)
                            {
                                if (FileSystem::Contains(cand))
                                    return cand;
                            }
                            return std::nullopt;
                        }
                        CFRelease(url);
                    }
                    CFRelease(font);
                }
                CFRelease(desc);
            }
            CFRelease(cfName);
        }
        for (const auto& cand : candidateNames)
        {
            if (FileSystem::Contains(cand))
                return cand;
        }
        return std::nullopt;
    }

    std::optional<std::string> FontResolver::ResolveDefault()
    {
        CTFontRef sysFont = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 0.0, nullptr);
        if (sysFont)
        {
            CFURLRef url = static_cast<CFURLRef>(CTFontCopyAttribute(sysFont, kCTFontURLAttribute));
            if (url)
            {
                char path[PATH_MAX];
                if (CFURLGetFileSystemRepresentation(url, true, reinterpret_cast<UInt8*>(path), sizeof(path)))
                {
                    std::string full(path);
                    CFRelease(url);
                    CFRelease(sysFont);
                    if (FileSystem::Contains(full))
                        return full;
                }
                CFRelease(url);
            }
            CFRelease(sysFont);
        }

        if (auto p = Resolve("San Francisco"))
            return p;

        if (auto p = Resolve("Helvetica Neue"))
            return p;

        if (auto p = Resolve("Helvetica"))
            return p;

        return std::nullopt;
    }
}


