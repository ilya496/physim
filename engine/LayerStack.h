#pragma once

#include <vector>
#include <memory>
#include "Layer.h"
#include "Event.h"

class LayerStack
{
public:
    LayerStack() = default;
    ~LayerStack();

    void PushLayer(std::unique_ptr<Layer> layer);
    void PopLayer(Layer* layer);

    void OnUpdate(float dt);
    void OnFixedUpdate(float fixedDt);
    void OnRender();
    void Broadcast(Event& event);

private:
    std::vector<std::unique_ptr<Layer>> m_Layers;
};
