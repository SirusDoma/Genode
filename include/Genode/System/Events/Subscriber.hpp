#pragma once

#include <Genode/System/Events/EventHandler.hpp>
#include <Genode/System/Events/Registration.hpp>

#include <functional>
#include <tuple>
#include <type_traits>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    /// @brief Represents the type-erased base of the subscribers
    ///
    /// User code never names this type: subscribers are declared
    /// through the `Subscriber` alias and bind to this base where
    /// the dispatcher is agnostic of their types (e.g. `Off`).
    ///
    /// @see `Subscriber`, `EventDispatcher`
    ////////////////////////////////////////////////////////////
    class SubscriberBase
    {
    public:
        SubscriberBase(const SubscriberBase&)            = delete;
        SubscriberBase& operator=(const SubscriberBase&) = delete;

        ////////////////////////////////////////////////////////////
        /// @brief Determines whether the subscriber is still registered
        ///
        /// @return `true` if the subscriber is still registered, otherwise `false`
        ////////////////////////////////////////////////////////////
        [[nodiscard]] bool IsActive() const noexcept;

        ////////////////////////////////////////////////////////////
        /// @brief Removes the subscriber from its dispatcher
        ///
        /// Does nothing if the subscriber is empty or already
        /// unsubscribed.
        ////////////////////////////////////////////////////////////
        void Unsubscribe() noexcept;

    protected:
        friend class EventDispatcher;

        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new empty instance
        ////////////////////////////////////////////////////////////
        SubscriberBase() = default;

        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new empty instance with an unsubscription callback
        ///
        /// The callback is invoked once when the subscriber
        /// unsubscribes itself and cannot be altered afterwards.
        ////////////////////////////////////////////////////////////
        explicit SubscriberBase(std::function<void()> onUnsubscribe) :
            m_unsubscribeCallback(std::move(onUnsubscribe))
        {
        }

        ////////////////////////////////////////////////////////////
        /// @brief Destroys the subscriber, unsubscribing it if still registered
        ////////////////////////////////////////////////////////////
        ~SubscriberBase()
        {
            Unsubscribe();
        }

        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new instance that takes over the registration of another subscriber
        ////////////////////////////////////////////////////////////
        SubscriberBase(SubscriberBase&& other) noexcept;

        ////////////////////////////////////////////////////////////
        /// @brief Takes over the registration of another subscriber, unsubscribing the current one first
        ////////////////////////////////////////////////////////////
        SubscriberBase& operator=(SubscriberBase&& other) noexcept;

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        EventDispatcher* m_dispatcher{};  //!< Dispatcher the subscriber is registered on, `nullptr` when inactive
        Registration     m_registration;  //!< Identity of the subscription within the dispatcher

    private:
        std::function<void()> m_unsubscribeCallback;  //!< Invoked once upon unsubscription
    };

    ////////////////////////////////////////////////////////////
    /// @brief Represents the subscription storage for a canonical key and argument list
    ///
    /// User code declares subscribers through the `Subscriber`
    /// alias, which canonicalizes the key and argument types to
    /// the ones this class is instantiated with.
    ///
    /// @see `Subscriber`, `EventDispatcher::On`
    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    class SubscriberImpl final : public SubscriberBase
    {
    public:
        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new empty instance
        ////////////////////////////////////////////////////////////
        SubscriberImpl() = default;

        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new instance that takes over the registration of another subscriber
        ////////////////////////////////////////////////////////////
        SubscriberImpl(SubscriberImpl&&) noexcept = default;

        ////////////////////////////////////////////////////////////
        /// @brief Takes over the registration of another subscriber, unsubscribing the current one first
        ////////////////////////////////////////////////////////////
        SubscriberImpl& operator=(SubscriberImpl&&) noexcept = default;

    private:
        friend class EventDispatcher;

        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new instance registered on the specified dispatcher
        ////////////////////////////////////////////////////////////
        SubscriberImpl(EventDispatcher& dispatcher, TKey key, EventHandler<TArgs...> handler,
                       std::function<void()> onUnsubscribe = nullptr);

        ////////////////////////////////////////////////////////////
        /// @brief Determines whether the subscriber is registered for the specified key
        ////////////////////////////////////////////////////////////
        [[nodiscard]] bool Matches(const TKey& key) const;

        ////////////////////////////////////////////////////////////
        /// @brief Invokes the handler with the dispatched arguments
        ////////////////////////////////////////////////////////////
        void Invoke(TArgs&... arguments);

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        TKey                   m_key{};    //!< Key the subscriber is registered for
        EventHandler<TArgs...> m_handler;  //!< The handler to invoke with dispatched arguments
    };

    ////////////////////////////////////////////////////////////
    /// @brief Represents an active subscription to events of a matching key and argument list
    ///
    /// This is the type returned by `EventDispatcher::On`: it owns
    /// the handler and unsubscribes it from the dispatcher when
    /// destroyed. It is movable but not copyable, and a subscriber
    /// that outlives its dispatcher simply becomes inactive.
    ///
    /// The key and argument types may be spelled exactly as the
    /// handler takes them, they are canonicalized to the stored
    /// types:
    /// @code
    /// Gx::Subscriber<const char*, const std::string&, int> m_subscriber;
    ///
    /// m_subscriber = dispatcher.On("key", [](const std::string& text, int value) { ... });
    /// @endcode
    ///
    /// @see `EventDispatcher::On`
    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    using Subscriber = SubscriberImpl<priv::CanonicalType<TKey>, priv::CanonicalType<TArgs>...>;

    namespace priv
    {
        template <typename TKey, typename TArguments>
        struct SubscriberFor;

        template <typename TKey, typename... TArgs>
        struct SubscriberFor<TKey, std::tuple<TArgs...>>
        {
            using Type = Subscriber<TKey, TArgs...>;
        };

        template <typename TKey, typename THandler>
        using SubscriberType = typename SubscriberFor<TKey, typename EventHandlerTraits<std::decay_t<THandler>>::Arguments>::Type;
    }
}
