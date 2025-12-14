#include "RenderLayer.h"

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream>
#include "EventBus.h"

RenderLayer::RenderLayer(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
}

void RenderLayer::OnAttach()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    CreateFramebuffer(m_Width, m_Height);
}

void RenderLayer::OnDetach()
{
    glDeleteFramebuffers(1, &m_Framebuffer);
}

void RenderLayer::OnRender()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);
    glViewport(0, 0, m_Width, m_Height);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.39f, 0.58f, 0.93f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    NewFrameRenderedEvent e = NewFrameRenderedEvent(
        m_ColorAttachment,
        m_Width,
        m_Height
    );
    EventBus::Publish(e);
}

void RenderLayer::CreateFramebuffer(uint32_t width, uint32_t height)
{
    glGenFramebuffers(1, &m_Framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffer);

    glGenTextures(1, &m_ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        m_ColorAttachment,
        0);

    // assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}