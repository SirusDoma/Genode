#pragma once

#include <typeindex>

#include <cstddef>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    /// @brief Represents the identity of a subscription within a dispatcher
    ///
    /// Subscribers are grouped by this identity, so the same key
    /// can be subscribed with different argument lists side by side.
    ////////////////////////////////////////////////////////////
    struct Registration
    {
        std::type_index Signature{typeid(void)}; //!< Type identity of the key and argument types
        std::size_t     Key{};                   //!< Hash of the key value

        bool operator==(const Registration& other) const noexcept
        {
            return Signature == other.Signature && Key == other.Key;
        }
    };

    ////////////////////////////////////////////////////////////
    /// @brief Provides a hash function for `Registration`
    ////////////////////////////////////////////////////////////
    struct RegistrationHasher
    {
        std::size_t operator()(const Registration& registration) const noexcept
        {
            return registration.Signature.hash_code() ^ (registration.Key + 0x9e3779b9 + (registration.Signature.hash_code() << 6));
        }
    };
}
