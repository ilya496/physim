#pragma once

#include "Event.h"

class Layer
{
public:
    virtual ~Layer() = default;

    virtual void OnAttach() {};
    virtual void OnDetach() {};
    virtual void OnUpdate(float dt) {};
    virtual void OnFixedUpdate(float fixedDt) {};
    virtual void OnEvent(Event& event) {};
    virtual void OnRender() {};
};