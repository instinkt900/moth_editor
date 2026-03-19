#pragma once

#include "moth_ui/events/event_listener.h"
#include "moth_ui/events/event_mouse.h"

class BoundsWidget;

struct BoundsHandleAnchor {
    bool Top = false;
    bool Left = false;
    bool Bottom = false;
    bool Right = false;
};

class BoundsHandle : public moth_ui::EventListener {
public:
    BoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    ~BoundsHandle() override;

    virtual void SetTarget(moth_ui::Node* node);

    bool OnEvent(moth_ui::Event const& event) override;
    virtual void Draw() = 0;

    static BoundsHandleAnchor constexpr TopLeft{ true, true, false, false };
    static BoundsHandleAnchor constexpr TopRight{ true, false, false, true };
    static BoundsHandleAnchor constexpr BottomLeft{ false, true, true, false };
    static BoundsHandleAnchor constexpr BottomRight{ false, false, true, true };
    static BoundsHandleAnchor constexpr Top{ true, false, false, false };
    static BoundsHandleAnchor constexpr Left{ false, true, false, false };
    static BoundsHandleAnchor constexpr Bottom{ false, false, true, false };
    static BoundsHandleAnchor constexpr Right{ false, false, false, true };

protected:
    BoundsWidget& m_widget;

    moth_ui::Node* m_target = nullptr;
    BoundsHandleAnchor m_anchor;
    moth_ui::FloatVec2 m_position;
    bool m_holding = false;

    virtual bool IsInBounds(moth_ui::IntVec2 const& pos) const = 0;

    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
    bool OnMouseMove(moth_ui::EventMouseMove const& event);
    virtual void UpdatePosition(moth_ui::IntVec2 const& position) = 0;
};
