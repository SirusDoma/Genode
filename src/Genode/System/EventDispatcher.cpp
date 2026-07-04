#include <Genode/System/EventDispatcher.hpp>

#include <algorithm>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    EventDispatcher::~EventDispatcher()
    {
        for (auto& [registration, subscribers] : m_subscriptions)
        {
            for (auto* subscriber : subscribers)
                subscriber->m_dispatcher = nullptr;
        }
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::Off(SubscriberBase& subscriber) noexcept
    {
        if (subscriber.m_dispatcher == this)
            subscriber.Unsubscribe();
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::Subscribe(const Registration& registration, SubscriberBase& subscriber)
    {
        m_subscriptions[registration].push_back(&subscriber);
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::Unsubscribe(SubscriberBase& subscriber) noexcept
    {
        const auto it = m_subscriptions.find(subscriber.m_registration);
        if (it == m_subscriptions.end())
            return;

        auto& subscribers = it->second;
        const auto entry = std::find(subscribers.begin(), subscribers.end(), &subscriber);
        if (entry == subscribers.end())
            return;

        subscribers.erase(entry);
        if (subscribers.empty())
            m_subscriptions.erase(it);
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::Rebind(SubscriberBase& from, SubscriberBase& to) noexcept
    {
        const auto it = m_subscriptions.find(to.m_registration);
        if (it == m_subscriptions.end())
            return;

        const auto entry = std::find(it->second.begin(), it->second.end(), &from);
        if (entry != it->second.end())
            *entry = &to;
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::Publish(const Registration& registration, const Publisher& publisher)
    {
        const auto it = m_subscriptions.find(registration);
        if (it == m_subscriptions.end())
            return;

        // Iterate a snapshot so handlers may subscribe, unsubscribe or dispatch
        // further events while a dispatch is in progress; subscribers registered
        // during the dispatch are not invoked for the event being delivered
        const auto subscribers = it->second;
        for (auto* subscriber : subscribers)
            publisher(*subscriber);
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::Store(QueuedEvent&& event)
    {
        const std::lock_guard lock(m_queueMutex);
        m_queue.push_back(std::move(event));
    }


    ////////////////////////////////////////////////////////////
    void EventDispatcher::DispatchQueue()
    {
        std::vector<QueuedEvent> queue;

        {
            const std::lock_guard lock(m_queueMutex);
            queue.swap(m_queue);
        }

        for (const auto& event : queue)
            event();
    }
}
