#include <Genode/Events/EventDispatcher.hpp>

#include <utility>

namespace Gx::Events
{
    ////////////////////////////////////////////////////////////
    bool SubscriberBase::IsActive() const noexcept
    {
        return m_dispatcher != nullptr;
    }


    ////////////////////////////////////////////////////////////
    void SubscriberBase::Unsubscribe() noexcept
    {
        if (m_dispatcher)
        {
            std::exchange(m_dispatcher, nullptr)->Unsubscribe(*this);

            if (const auto callback = std::exchange(m_unsubscribeCallback, nullptr))
                callback();
        }
    }


    ////////////////////////////////////////////////////////////
    SubscriberBase::SubscriberBase(SubscriberBase&& other) noexcept :
        m_dispatcher(std::exchange(other.m_dispatcher, nullptr)),
        m_registration(other.m_registration),
        m_unsubscribeCallback(std::exchange(other.m_unsubscribeCallback, nullptr))
    {
        if (m_dispatcher)
            m_dispatcher->Rebind(other, *this);
    }


    ////////////////////////////////////////////////////////////
    SubscriberBase& SubscriberBase::operator=(SubscriberBase&& other) noexcept
    {
        if (this != &other)
        {
            Unsubscribe();

            m_dispatcher    = std::exchange(other.m_dispatcher, nullptr);
            m_registration  = other.m_registration;
            m_unsubscribeCallback = std::exchange(other.m_unsubscribeCallback, nullptr);

            if (m_dispatcher)
                m_dispatcher->Rebind(other, *this);
        }

        return *this;
    }
}
