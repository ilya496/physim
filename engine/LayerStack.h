#pragma once

#include <vector>
#include <memory>
#include "Layer.h"

class EventDispatcher
{
public:
    virtual ~EventDispatcher() = default;
    virtual void dispatchEvent(std::shared_ptr<Event> event) = 0;
};

class LayerStack : EventDispatcher
{
public:
    LayerStack() = default;
    ~LayerStack();

    void PushLayer(std::shared_ptr<Layer> layer);
    void PopLayer(std::shared_ptr<Layer> layer);

    void OnUpdate();
    void OnDetach();
    virtual void dispatchEvent(std::shared_ptr<Event> event) override;

private:
    std::vector<std::shared_ptr<Layer>> m_Layers;
};