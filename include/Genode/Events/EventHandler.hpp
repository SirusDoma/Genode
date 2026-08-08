#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    /// @brief Represents the function that handles events dispatched with @a `TArgs` arguments
    ///
    /// The arguments are passed as lvalues, so a handler may
    /// modify them and later subscribers of the same event will
    /// observe the modifications.
    ////////////////////////////////////////////////////////////
    template <typename... TArgs>
    using EventHandler = std::function<void(TArgs&...)>;

    namespace priv
    {
        ////////////////////////////////////////////////////////////
        /// @brief Represents the canonical storage type of a key or argument
        ///
        /// String-like types canonicalize to `std::string` so they
        /// compare by content, every other type decays.
        ////////////////////////////////////////////////////////////
        template <typename T>
        using CanonicalType = std::conditional_t<std::is_convertible_v<std::decay_t<T>, std::string_view>, std::string, std::decay_t<T>>;

        ////////////////////////////////////////////////////////////
        /// @brief Represents how a dispatched argument travels to the subscribers
        ///
        /// A non-const lvalue of its canonical type is passed by
        /// reference, anything else is materialized into a copy of
        /// the canonical type that lives until the event is delivered.
        ////////////////////////////////////////////////////////////
        template <typename T>
        using ForwardedType = std::conditional_t<
            std::is_lvalue_reference_v<T> && std::is_same_v<std::remove_reference_t<T>, CanonicalType<T>>,
            CanonicalType<T>&,
            CanonicalType<T>>;

        template <typename THandler, typename = void>
        struct EventHandlerTraits
        {
        };

        template <typename THandler>
        struct EventHandlerTraits<THandler, std::void_t<decltype(&THandler::operator())>>
            : EventHandlerTraits<decltype(&THandler::operator())>
        {
        };

        template <typename TReturn, typename... TArgs>
        struct EventHandlerTraits<TReturn (*)(TArgs...)>
        {
            using Arguments = std::tuple<TArgs...>;
        };

        template <typename TReturn, typename... TArgs>
        struct EventHandlerTraits<TReturn (*)(TArgs...) noexcept>
        {
            using Arguments = std::tuple<TArgs...>;
        };

        template <typename TClass, typename TReturn, typename... TArgs>
        struct EventHandlerTraits<TReturn (TClass::*)(TArgs...)>
        {
            using Arguments = std::tuple<TArgs...>;
        };

        template <typename TClass, typename TReturn, typename... TArgs>
        struct EventHandlerTraits<TReturn (TClass::*)(TArgs...) const>
        {
            using Arguments = std::tuple<TArgs...>;
        };

        template <typename TClass, typename TReturn, typename... TArgs>
        struct EventHandlerTraits<TReturn (TClass::*)(TArgs...) noexcept>
        {
            using Arguments = std::tuple<TArgs...>;
        };

        template <typename TClass, typename TReturn, typename... TArgs>
        struct EventHandlerTraits<TReturn (TClass::*)(TArgs...) const noexcept>
        {
            using Arguments = std::tuple<TArgs...>;
        };
    }
}
