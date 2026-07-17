#pragma once

#include <SFML/System/String.hpp>

#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>

namespace Gx
{
    class StringHelper
    {
    public:
        [[nodiscard]] static bool IsGlobMatch(const std::string& input, const sf::String& pattern, bool caseSensitive = true);

        [[nodiscard]] static bool EqualsCaseInsensitive(const std::string& a, const std::string& b);

        [[nodiscard]] static sf::String TrimStart(const sf::String& input);
        [[nodiscard]] static sf::String TrimEnd(const sf::String& input);
        [[nodiscard]] static sf::String Trim(const sf::String& input);

        [[nodiscard]] static std::string RemoveExtension(const std::string& fileName);
        [[nodiscard]] static std::string GetTypeName(const std::type_info& type, bool withNamespace = true);

        [[nodiscard]] static bool StartsWith(const std::string& string, const std::string& prefix);
        [[nodiscard]] static bool EndsWith(const std::string& string, const std::string& suffix);

        [[nodiscard]] static sf::String ToPascalCase(const sf::String& input);
        [[nodiscard]] static std::vector<std::string> Split(const std::string& input, char delimiter = ' ');
        [[nodiscard]] static sf::String Unquote(const sf::String& string);

        [[nodiscard]] static sf::String ToString(int value, int totalLength = 0);
        [[nodiscard]] static sf::String ToString(float value, int precision = 0);

        template<typename T>
        [[nodiscard]] static std::string ToString(const T& value)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return value;
            }
            else if constexpr (std::is_same_v<T, sf::String>)
            {
                return value.toAnsiString();
            }
            else
            {
                std::stringstream ss;
                ss << value;

                return ss.str();
            }
        }

        template<typename T>
        [[nodiscard]] static std::string GetTypeName(T& obj, const bool withNamespace = true)
        {
            return GetTypeName(typeid(obj), withNamespace);
        }

        template<typename T>
        [[nodiscard]] static std::string GetTypeName(const bool withNamespace = true)
        {
            return GetTypeName(typeid(T), withNamespace);
        }
    };
}
