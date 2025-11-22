#pragma once

#include "Event.h"

class Layer
{
public:
    virtual ~Layer() = default;

    virtual void OnAttach() {};
    virtual void OnDetach() {};
    virtual void OnUpdate() {};
    virtual void OnEvent(Event& event) {};
};