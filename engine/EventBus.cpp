#include "EventBus.h"

EventBus::Subscription::Subscription(
    std::type_index type,
    size_t index,
    std::function<void(std::type_index, size_t)> unsubscribeFn)
    : m_Type(type),
    m_Index(index),
    m_UnsubscribeFn(std::move(unsubscribeFn)),
    m_Active(true)
{
}

EventBus::Subscription::Subscription(Subscription&& other) noexcept
{
    *this = std::move(other);
}

EventBus::Subscription&
EventBus::Subscription::operator=(Subscription&& other) noexcept
{
    if (this != &other)
    {
        Unsubscribe();
        m_Type = other.m_Type;
        m_Index = other.m_Index;
        m_UnsubscribeFn = std::move(other.m_UnsubscribeFn);
        m_Active = other.m_Active;

        other.m_Active = false;
    }
    return *this;
}

EventBus::Subscription::~Subscription()
{
    Unsubscribe();
}

void EventBus::Subscription::Unsubscribe()
{
    if (m_Active && m_UnsubscribeFn)
    {
        m_UnsubscribeFn(m_Type, m_Index);
        m_Active = false;
    }
}
