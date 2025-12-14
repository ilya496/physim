#pragma once

#include "Layer.h"

class RenderLayer : public Layer
{
public:
    RenderLayer(uint32_t width, uint32_t height);

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnRender() override;

private:
    void CreateFramebuffer(uint32_t width, uint32_t height);

private:
    uint32_t m_Framebuffer = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_Width, m_Height;
};