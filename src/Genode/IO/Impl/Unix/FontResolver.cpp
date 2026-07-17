#include <Genode/IO/Impl/Unix/FontResolver.hpp>
#include <Genode/IO/FileSystem.hpp>

#include <vector>

#include <unistd.h>
#ifdef USE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

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

#ifdef USE_FONTCONFIG
        if (FcInit())
        {
            FcPattern* pat = FcNameParse(reinterpret_cast<const FcChar8*>(nameOrPath.c_str()));
            if (pat)
            {
                FcConfigSubstitute(nullptr, pat, FcMatchPattern);
                FcDefaultSubstitute(pat);
                FcResult result;
                FcPattern* font = FcFontMatch(nullptr, pat, &result);
                if (font)
                {
                    FcChar8* file = nullptr;
                    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch && file)
                    {
                        std::string full(reinterpret_cast<const char*>(file));
                        FcPatternDestroy(font);
                        FcPatternDestroy(pat);
                        if (FileSystem::Contains(full))
                            return full;
                    }
                    FcPatternDestroy(font);
                }
                FcPatternDestroy(pat);
            }
        }
#endif
        for (const auto& cand : candidateNames)
        {
            if (FileSystem::Contains(cand))
                return cand;
        }
        return std::nullopt;
    }

    std::optional<std::string> FontResolver::ResolveDefault()
    {
#ifdef USE_FONTCONFIG
        if (FcInit())
        {
            FcPattern* pat = FcPatternBuild(nullptr,
                                            FC_FAMILY, FcTypeString, "sans",
                                            FC_WEIGHT, FcTypeInteger, FC_WEIGHT_NORMAL,
                                            FC_SLANT,  FcTypeInteger, FC_SLANT_ROMAN,
                                            nullptr);
            if (pat)
            {
                FcConfigSubstitute(nullptr, pat, FcMatchPattern);
                FcDefaultSubstitute(pat);
                FcResult result;
                FcPattern* font = FcFontMatch(nullptr, pat, &result);
                if (font)
                {
                    FcChar8* file = nullptr;
                    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch && file)
                    {
                        std::string full(reinterpret_cast<const char*>(file));
                        FcPatternDestroy(font);
                        FcPatternDestroy(pat);
                        if (FileSystem::Contains(full))
                            return full;
                    }
                    FcPatternDestroy(font);
                }
                FcPatternDestroy(pat);
            }
        }
#endif

        if (auto p = Resolve("DejaVu Sans"))
            return p;

        if (auto p = Resolve("Liberation Sans"))
            return p;

        return std::nullopt;
    }
}


