#pragma once

enum struct EventType {
    KEY_PRESS_EVENT,
    KEY_RELEASE_EVENT,
    KEY_REPEAT_EVENT,

    MOUSE_MOVE_EVENT,
    MOUSE_BUTTON_PRESS_EVENT,
    MOUSE_BUTTON_RELEASE_EVENT,
    MOUSE_SCROLL_EVENT,

    NEW_FRAME_RENDERED_EVENT,
    UPDATE_RENDER_SETTINGS_EVENT,

    WINDOW_RESIZE_EVENT,
};

class Event {
public:
    virtual ~Event() = default;
    virtual EventType GetType() const = 0;
    virtual bool IsInputEvent() const { return false; }

    void Consume() { m_IsConsumed = true; }
    bool IsConsumed() const { return m_IsConsumed; }
private:
    bool m_IsConsumed = false;
};