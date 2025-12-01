#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <typeindex>
#include <memory>

class EventBus
{
public:
    class Subscription
    {
    public:
        Subscription() = default;

        Subscription(std::type_index type,
            size_t index,
            std::function<void(std::type_index, size_t)> unsubscribeFn);

        Subscription(Subscription&& other) noexcept;
        Subscription& operator=(Subscription&& other) noexcept;

        ~Subscription();

        void Unsubscribe();

    private:
        bool m_Active = false;
        std::type_index m_Type{ typeid(void) };
        size_t m_Index = 0;
        std::function<void(std::type_index, size_t)> m_UnsubscribeFn;
    };

    template<typename EventT>
    static Subscription Subscribe(std::function<void(const EventT&)> callback)
    {
        auto& listeners = GetListeners<EventT>();
        listeners.push_back(std::move(callback));
        size_t index = listeners.size() - 1;

        return Subscription(
            typeid(EventT), index,
            [](std::type_index, size_t slot) {
                RemoveListener<EventT>(slot);
            }
        );
    }

    template<typename EventT>
    static void Publish(const EventT& event)
    {
        auto& listeners = GetListeners<EventT>();
        for (auto& listener : listeners)
        {
            if (listener)
                listener(event);
        }
    }

private:
    template<typename EventT>
    static std::vector<std::function<void(const EventT&)>>& GetListeners()
    {
        static std::vector<std::function<void(const EventT&)>> listeners;
        return listeners;
    }

    template<typename EventT>
    static void RemoveListener(size_t index)
    {
        auto& listeners = GetListeners<EventT>();
        if (index < listeners.size())
            listeners[index] = nullptr;
    }
};
