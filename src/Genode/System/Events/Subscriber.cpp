#include <Genode/System/EventDispatcher.hpp>

#include <utility>

namespace Gx
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
            std::exchange(m_dispatcher, nullptr)->Unsubscribe(*this);
    }


    ////////////////////////////////////////////////////////////
    SubscriberBase::SubscriberBase(SubscriberBase&& other) noexcept :
        m_dispatcher(std::exchange(other.m_dispatcher, nullptr)),
        m_registration(other.m_registration)
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

            m_dispatcher   = std::exchange(other.m_dispatcher, nullptr);
            m_registration = other.m_registration;

            if (m_dispatcher)
                m_dispatcher->Rebind(other, *this);
        }

        return *this;
    }
}
