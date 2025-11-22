#pragma once

enum class EventType {
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
};

class Event {
public:
    virtual ~Event() = default;

    virtual EventType GetType() const = 0;

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

    int Width, Height;
};

class WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() {}

    static EventType GetStaticType() { return EventType::WINDOW_CLOSE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }
};

class KeyPressEvent : public Event
{
public:
    KeyPressEvent(int key, bool repeat)
        : KeyCode(key), IsRepeat(repeat) {
    }

    static EventType GetStaticType() { return EventType::KEY_PRESS_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }

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

    int Button;
};

class MouseMoveEvent : public Event
{
public:
    MouseMoveEvent(float x, float y)
        : X(x), Y(y) {
    }

    static EventType GetStaticType() { return EventType::MOUSE_MOVE_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }

    float X, Y;
};

class MouseScrollEvent : public Event
{
public:
    MouseScrollEvent(float x, float y)
        : X(x), Y(y) {
    }

    static EventType GetStaticType() { return EventType::MOUSE_SCROLL_EVENT; }
    virtual EventType GetType() const override { return GetStaticType(); }

    float X, Y;
};