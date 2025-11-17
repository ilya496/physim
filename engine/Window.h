#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

struct WindowProps {
    std::string title;
    int width;
    int height;
    bool VSync;

    WindowProps(const std::string& title = std::string("Physim"),
        int width = 1920,
        int height = 1080,
        bool VSync = false)
        : width(width), height(height), title(title), VSync(VSync) {
    }
};

class Window
{
public:
    virtual ~Window() = default;
    virtual void OnUpdate() = 0;

    virtual void SetTitle(const std::string& title) = 0;
    virtual bool IsVSync() const = 0;
    virtual void SetVSync(bool enabled) = 0;
    virtual glm::ivec2 GetFramebufferSize() const = 0;
    virtual void* GetNativeWindow() const = 0;
    virtual bool IsFullscreen() const = 0;
    virtual void SetFullscreen(bool enabled) = 0;

    virtual void Minimize() = 0;
    virtual void Maximize() = 0;
    virtual void Restore() = 0;
    virtual bool IsMaximized() const = 0;
    virtual void Close() = 0;
    virtual void SetPosition(int x, int y) = 0;
    virtual glm::ivec2 GetPosition() const = 0;

    // virtual bool IsKeyPressed(KeyCode key) = 0;
    // virtual bool IsMouseButtonPressed(MouseCode key) = 0;
    // virtual glm::vec2 GetMousePosition() = 0;

    virtual bool ShouldClose() = 0;

    // virtual void SetEventCallback(const std::function<void(std::shared_ptr<Event>)>& callback) = 0;

    static std::shared_ptr<Window> Create(const WindowProps props);
};