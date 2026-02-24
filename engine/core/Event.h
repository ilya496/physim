#pragma once

#include <string>
#include <vector>

enum class EventType {
    None = 0,
    KEY_PRESS_EVENT,
    KEY_RELEASE_EVENT,
    KEY_REPEAT_EVENT,

    MOUSE_MOVE_EVENT,
    MOUSE_BUTTON_PRESS_EVENT,
    MOUSE_BUTTON_RELEASE_EVENT,
    MOUSE_SCROLL_EVENT,

    WINDOW_RESIZE_EVENT,
    WINDOW_CLOSE_EVENT,
    NEW_FRAME_RENDERED_EVENT,
    UPDATE_RENDER_SETTINGS_EVENT,

    VIEWPORT_EVENT,
    REQUEST_FRAME_CAPTURE_EVENT,
};

class Event {
public:
    virtual ~Event() = default;

    virtual EventType GetType() const = 0;
    virtual const char* GetName() const = 0;
    virtual std::string ToString() const { return GetName(); }

    bool IsConsumed() const { return m_IsConsumed; }
    void Consume() { m_IsConsumed = true; }
protected:
    bool m_IsConsumed = false;
};

class WindowResizeEvent : public Event
{
public:
    WindowResizeEvent(int w, int h)
        : Width(w), Height(h) {
    }

    static EventType GetStaticType() { return EventType::WINDOW_RESIZE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "WINDOW_RESIZE_EVENT"; }

    int Width, Height;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() {}

    static EventType GetStaticType() { return EventType::WINDOW_CLOSE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "WINDOW_CLOSE_EVENT"; }
};

class KeyPressEvent : public Event
{
public:
    KeyPressEvent(int key, bool repeat)
        : KeyCode(key), IsRepeat(repeat) {
    }

    static EventType GetStaticType() { return EventType::KEY_PRESS_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KEY_PRESS_EVENT"; }

    int KeyCode;
    bool IsRepeat;
};

class KeyReleaseEvent : public Event
{
public:
    KeyReleaseEvent(int key)
        : KeyCode(key) {
    }

    static EventType GetStaticType() { return EventType::KEY_RELEASE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KEY_RELEASE_EVENT"; }

    int KeyCode;
};

class MouseButtonPressEvent : public Event
{
public:
    MouseButtonPressEvent(int button)
        : Button(button) {
    }

    static EventType GetStaticType() { return EventType::MOUSE_BUTTON_PRESS_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MOUSE_BUTTON_PRESS_EVENT"; }

    int Button;
};

class MouseButtonReleaseEvent : public Event
{
public:
    MouseButtonReleaseEvent(int button)
        : Button(button) {
    }

    static EventType GetStaticType() { return EventType::MOUSE_BUTTON_RELEASE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MOUSE_BUTTON_RELEASE_EVENT"; }

    int Button;
};

class MouseMoveEvent : public Event
{
public:
    MouseMoveEvent(float x, float y, float dx, float dy)
        : X(x), Y(y), DeltaX(dx), DeltaY(dy) {
    }

    static EventType GetStaticType() { return EventType::MOUSE_MOVE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MOUSE_MOVE_EVENT"; }

    float X, Y;
    float DeltaX, DeltaY;
};

class MouseScrollEvent : public Event
{
public:
    MouseScrollEvent(float x, float y)
        : X(x), Y(y) {
    }

    static EventType GetStaticType() { return EventType::MOUSE_SCROLL_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MOUSE_SCROLL_EVENT"; }

    float X, Y;
};

class NewFrameRenderedEvent : public Event
{
public:
    NewFrameRenderedEvent() {}
    NewFrameRenderedEvent(uint32_t colorAttachment, uint32_t width, uint32_t height)
        : ColorAttachment(colorAttachment), Width(width), Height(height) {
    }

    static EventType GetStaticType() { return EventType::NEW_FRAME_RENDERED_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "NEW_FRAME_RENDERED_EVENT"; }

    uint32_t ColorAttachment;
    uint32_t Width, Height;
    std::vector<uint8_t>* PixelData = nullptr;
};

class ViewportEvent : public Event
{
public:
    ViewportEvent(
        float mouseX,
        float mouseY,
        float viewportX,
        float viewportY,
        float viewportWidth,
        float viewportHeight,
        bool hovered
    ) : MouseX(mouseX), MouseY(mouseY), ViewportX(viewportX), ViewportY(viewportY),
        ViewportWidth(viewportWidth), ViewportHeight(viewportHeight), Hovered(hovered)
    {
    }

    static EventType GetStaticType() { return EventType::VIEWPORT_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "VIEWPORT_EVENT"; }

    float MouseX;
    float MouseY;
    float ViewportX;
    float ViewportY;
    float ViewportWidth;
    float ViewportHeight;
    bool Hovered;
};

class RequestFrameCaptureEvent : public Event
{
public:
    RequestFrameCaptureEvent(bool capture)
        : CapturePixels(capture)
    {
    }

    static EventType GetStaticType() { return EventType::REQUEST_FRAME_CAPTURE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "REQUEST_FRAME_CAPTURE_EVENT"; }

    bool CapturePixels = true;
};