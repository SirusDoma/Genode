#pragma once

#include <Genode/System/EventDispatcher.hpp>

#include <utility>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    SubscriberImpl<TKey, TArgs...>::SubscriberImpl(
        EventDispatcher& dispatcher,
        TKey key, EventHandler<TArgs...> handler,
        std::function<void()> onUnsubscribe
    ) : SubscriberBase(std::move(onUnsubscribe)),
        m_key(std::move(key)),
        m_handler(std::move(handler))
    {
        m_dispatcher   = &dispatcher;
        m_registration = Registration{typeid(SubscriberImpl), std::hash<TKey>{}(m_key)};
        dispatcher.Subscribe(m_registration, *this);
    }


    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    bool SubscriberImpl<TKey, TArgs...>::Matches(const TKey& key) const
    {
        return m_key == key;
    }


    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    void SubscriberImpl<TKey, TArgs...>::Invoke(TArgs&... arguments)
    {
        m_handler(arguments...);
    }


    ////////////////////////////////////////////////////////////
    template <typename TKey, typename THandler, typename>
    auto EventDispatcher::On(TKey&& key, THandler&& handler, std::function<void()> onUnsubscribe)
    {
        using Type = priv::SubscriberType<TKey, THandler>;

        return Type(*this, std::forward<TKey>(key), std::forward<THandler>(handler), std::move(onUnsubscribe));
    }


    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    void EventDispatcher::Dispatch(TKey&& key, TArgs&&... arguments)
    {
        const priv::CanonicalType<TKey> canonicalKey(std::forward<TKey>(key));
        std::tuple<priv::ForwardedType<TArgs>...> forwardedArguments(std::forward<TArgs>(arguments)...);

        Publish<priv::CanonicalType<TKey>, priv::CanonicalType<TArgs>...>(canonicalKey, forwardedArguments);
    }


    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs>
    void EventDispatcher::Enqueue(TKey&& key, TArgs&&... arguments)
    {
        Store([this,
              canonicalKey       = priv::CanonicalType<TKey>(std::forward<TKey>(key)),
              forwardedArguments = std::tuple<priv::ForwardedType<TArgs>...>(std::forward<TArgs>(arguments)...)]() mutable
             { Publish<priv::CanonicalType<TKey>, priv::CanonicalType<TArgs>...>(canonicalKey, forwardedArguments); });
    }


    ////////////////////////////////////////////////////////////
    template <typename TKey, typename... TArgs, typename TArguments>
    void EventDispatcher::Publish(const TKey& key, TArguments& arguments)
    {
        const Registration registration{typeid(SubscriberImpl<TKey, TArgs...>), std::hash<TKey>{}(key)};

        Publish(registration,
                [&](SubscriberBase& base)
                {
                    auto& subscriber = static_cast<SubscriberImpl<TKey, TArgs...>&>(base);
                    if (subscriber.Matches(key))
                        std::apply([&subscriber](TArgs&... unpacked) { subscriber.Invoke(unpacked...); }, arguments);
                });
    }
}
