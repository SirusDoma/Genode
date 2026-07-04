#pragma once

#include <Genode/System/Events/EventHandler.hpp>
#include <Genode/System/Events/Registration.hpp>
#include <Genode/System/Events/Subscriber.hpp>

#include <functional>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Gx
{
    ////////////////////////////////////////////////////////////
    /// @brief Provides a keyed event dispatching mechanism
    ///
    /// Subscribers are notified when an event is dispatched with
    /// an equal key of the same type and a matching argument list;
    /// the same key can be subscribed with different argument
    /// lists side by side.
    ///
    /// The dispatcher is not synchronized except for its event
    /// queue: `Enqueue` may be called from any thread.
    ///
    /// @see `Subscriber`
    ////////////////////////////////////////////////////////////
    class EventDispatcher
    {
    public:
        ////////////////////////////////////////////////////////////
        /// @brief Initializes a new instance of the `EventDispatcher` class
        ////////////////////////////////////////////////////////////
        EventDispatcher() = default;

        ////////////////////////////////////////////////////////////
        /// @brief Destroys the dispatcher, deactivating every subscriber still registered
        ////////////////////////////////////////////////////////////
        virtual ~EventDispatcher();

        EventDispatcher(const EventDispatcher&)            = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;
        EventDispatcher(EventDispatcher&&)                 = delete;
        EventDispatcher& operator=(EventDispatcher&&)      = delete;

        ////////////////////////////////////////////////////////////
        /// @brief Subscribes the specified handler to events dispatched with a matching key and argument list
        ///
        /// The argument list is deduced from the handler signature.
        /// Generic callables (e.g. lambdas taking `const auto&`)
        /// cannot be deduced.
        ///
        /// @param key     The key to subscribe to
        /// @param handler The handler invoked with the dispatched arguments
        ///
        /// @return A `Subscriber` of the key and handler argument types
        ///         that unsubscribes the handler when destroyed
        ///
        /// @see `Off`, `Dispatch`, `Enqueue`, `Subscriber`
        ////////////////////////////////////////////////////////////
        template <typename TKey, typename THandler, typename = std::void_t<typename priv::EventHandlerTraits<std::decay_t<THandler>>::Arguments>>
        [[nodiscard]] auto On(TKey&& key, THandler&& handler);

        ////////////////////////////////////////////////////////////
        /// @brief Unsubscribes the specified subscriber if it is registered on this dispatcher
        ///
        /// @param subscriber The subscriber to unsubscribe
        ///
        /// @see `On`
        ////////////////////////////////////////////////////////////
        void Off(SubscriberBase& subscriber) noexcept;

        ////////////////////////////////////////////////////////////
        /// @brief Dispatches an event synchronously to every subscriber with a matching key and argument list
        ///
        /// An argument passed as a non-const lvalue of its
        /// canonical type is passed by reference: subscribers may
        /// modify it and the modifications remain visible to the
        /// caller and to later subscribers. Any other argument is
        /// materialized into a copy for the duration of the call.
        /// The dispatcher takes no ownership: the caller is
        /// responsible for the validity of the references and
        /// pointers it passes.
        ///
        /// @param key       The key identifying the event
        /// @param arguments The event arguments to deliver
        ///
        /// @see `On`, `Enqueue`
        ////////////////////////////////////////////////////////////
        template <typename TKey, typename... TArgs>
        void Dispatch(TKey&& key, TArgs&&... arguments);

        ////////////////////////////////////////////////////////////
        /// @brief Enqueues an event until the queue is dispatched
        ///
        /// The arguments are forwarded like `Dispatch` forwards
        /// them: a non-const lvalue of its canonical type is
        /// stored by reference and must remain valid until
        /// `DispatchQueue` delivers the event, anything else is
        /// copied into the queue. This member is safe to call
        /// from any thread.
        ///
        /// @param key       The key identifying the event
        /// @param arguments The event arguments to deliver
        ///
        /// @see `DispatchQueue`, `Dispatch`
        ////////////////////////////////////////////////////////////
        template <typename TKey, typename... TArgs>
        void Enqueue(TKey&& key, TArgs&&... arguments);

        ////////////////////////////////////////////////////////////
        /// @brief Dispatches every enqueued event on the calling thread
        ///
        /// Delivers the events enqueued so far in their enqueue
        /// order. Events enqueued while the queue is dispatching
        /// (e.g. from a handler) are delivered by the next call.
        ///
        /// @see `Enqueue`
        ////////////////////////////////////////////////////////////
        virtual void DispatchQueue();

    protected:
        ////////////////////////////////////////////////////////////
        /// @brief Represents the delivery of a dispatched event to a subscriber
        ////////////////////////////////////////////////////////////
        using Publisher = std::function<void(SubscriberBase&)>;

        ////////////////////////////////////////////////////////////
        /// @brief Represents an event stored in the queue until it is dispatched
        ////////////////////////////////////////////////////////////
        using QueuedEvent = std::function<void()>;

        ////////////////////////////////////////////////////////////
        /// @brief Registers the specified subscriber under its registration identity
        ///
        /// @param registration The identity of the subscription
        /// @param subscriber   The subscriber to register
        ////////////////////////////////////////////////////////////
        virtual void Subscribe(const Registration& registration, SubscriberBase& subscriber);

        ////////////////////////////////////////////////////////////
        /// @brief Removes the specified subscriber from the dispatcher
        ///
        /// @param subscriber The subscriber to remove
        ////////////////////////////////////////////////////////////
        virtual void Unsubscribe(SubscriberBase& subscriber) noexcept;

        ////////////////////////////////////////////////////////////
        /// @brief Redirects the registration of a moved subscriber to its new instance
        ///
        /// @param from The subscriber that was moved from
        /// @param to   The subscriber that was moved to
        ////////////////////////////////////////////////////////////
        virtual void Rebind(SubscriberBase& from, SubscriberBase& to) noexcept;

        ////////////////////////////////////////////////////////////
        /// @brief Delivers a dispatched event to every subscriber of the specified registration
        ///
        /// @param registration The identity of the dispatched event
        /// @param publisher    The delivery invoked with each matching subscriber
        ////////////////////////////////////////////////////////////
        virtual void Publish(const Registration& registration, const Publisher& publisher);

        ////////////////////////////////////////////////////////////
        /// @brief Stores the specified event in the queue
        ///
        /// @param event The event to store until the queue is dispatched
        ////////////////////////////////////////////////////////////
        virtual void Store(QueuedEvent&& event);

    private:
        friend class SubscriberBase;
        template <typename, typename...> friend class SubscriberImpl;

        ////////////////////////////////////////////////////////////
        /// @brief Delivers a canonicalized event to the matching subscribers
        ////////////////////////////////////////////////////////////
        template <typename TKey, typename... TArgs, typename TArguments>
        void Publish(const TKey& key, TArguments& arguments);

        ////////////////////////////////////////////////////////////
        // Member data
        ////////////////////////////////////////////////////////////
        using SubscriptionMap = std::unordered_map<Registration, std::vector<SubscriberBase*>, RegistrationHasher>;

        SubscriptionMap          m_subscriptions; //!< Registered subscribers grouped by registration identity
        std::vector<QueuedEvent> m_queue;         //!< Events waiting for the queue to be dispatched
        std::mutex               m_queueMutex;    //!< Mutex guarding the event queue
    };
}

#include <Genode/System/EventDispatcher.inl>
