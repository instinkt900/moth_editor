#pragma once

#include "moth_ui/events/event.h"

enum EventType : int {
    EVENTTYPE_RENDERDEVICERESET = moth_ui::EventType::EVENTTYPE_USER,
    EVENTTYPE_RENDERTARGETRESET,
    EVENTTYPE_WINDOWSIZE,
    EVENTTYPE_REQUEST_QUIT,
    EVENTTYPE_QUIT,
};

class EventRenderDeviceReset : public moth_ui::Event {
public:
    EventRenderDeviceReset()
        : moth_ui::Event(GetStaticType()) {}
    virtual ~EventRenderDeviceReset() = default;

    static constexpr int GetStaticType() { return EVENTTYPE_RENDERDEVICERESET; }

    std::unique_ptr<Event> Clone() const override {
        return std::make_unique<EventRenderDeviceReset>();
    }
};

class EventRenderTargetReset : public moth_ui::Event {
public:
    EventRenderTargetReset()
        : moth_ui::Event(GetStaticType()) {}
    virtual ~EventRenderTargetReset() = default;

    static constexpr int GetStaticType() { return EVENTTYPE_RENDERTARGETRESET; }

    std::unique_ptr<Event> Clone() const override {
        return std::make_unique<EventRenderTargetReset>();
    }
};

class EventWindowSize : public moth_ui::Event {
public:
    EventWindowSize(int width, int height)
        : moth_ui::Event(GetStaticType())
        , m_width(width)
        , m_height(height) {}
    virtual ~EventWindowSize() = default;

    static constexpr int GetStaticType() { return EVENTTYPE_WINDOWSIZE; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    std::unique_ptr<Event> Clone() const override {
        return std::make_unique<EventWindowSize>(m_width, m_height);
    }

private:
    int m_width = 0;
    int m_height = 0;
};

class EventRequestQuit : public moth_ui::Event {
public:
    EventRequestQuit()
        : moth_ui::Event(GetStaticType()) {}
    virtual ~EventRequestQuit() = default;

    static constexpr int GetStaticType() { return EVENTTYPE_REQUEST_QUIT; }

    std::unique_ptr<Event> Clone() const override {
        return std::make_unique<EventRequestQuit>();
    }
};

class EventQuit : public moth_ui::Event {
public:
    EventQuit()
        : moth_ui::Event(GetStaticType()) {}
    virtual ~EventQuit() = default;

    static constexpr int GetStaticType() { return EVENTTYPE_QUIT; }

    std::unique_ptr<Event> Clone() const override {
        return std::make_unique<EventQuit>();
    }
};
