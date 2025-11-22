#include "LayerStack.h"

LayerStack::~LayerStack()
{
    for (auto& layer : m_Layers)
        layer->OnDetach();
}

void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
{
    layer->OnAttach();
    m_Layers.emplace_back(std::move(layer));
}

void LayerStack::PopLayer(Layer* layer)
{
    for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
    {
        if (it->get() == layer)
        {
            (*it)->OnDetach();
            m_Layers.erase(it);
            return;
        }
    }
}

void LayerStack::OnUpdate()
{
    for (auto& layer : m_Layers)
        layer->OnUpdate();
}

void LayerStack::Broadcast(Event& event)
{
    for (auto it = m_Layers.begin(); it != m_Layers.end(); ++it)
    {
        (*it)->OnEvent(event);
        if (event.IsConsumed())
            return;
    }
}