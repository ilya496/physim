#pragma once

#include <memory>

class Layer
{
public:
    virtual ~Layer() = default;

public:
    virtual void OnEvent() {}
    virtual void OnUpdate() {}
    virtual void OnRender() {}

    template <std::derived_from<Layer> T, typename... Args>
    void TransitionTo(Args&&... args)
    {
        QueueTransition(std::move(std::make_unique<T>(std::forward<Args>(args)...)));
    }

private:
    void QueueTransition(std::unique_ptr<Layer> layer);
};