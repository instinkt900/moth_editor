#pragma once

#include "moth/ui/events/event_listener.h"
#include "moth/ui/events/event_mouse.h"

class BoundsWidget;

struct BoundsHandleAnchor {
    bool Top = false;
    bool Left = false;
    bool Bottom = false;
    bool Right = false;
};

class BoundsHandle : public moth::ui::IEventListener {
public:
    BoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    ~BoundsHandle() override;

    virtual void SetTarget(moth::ui::Node* node);

    bool OnEvent(moth::ui::Event const& event) override;
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

    moth::ui::Node* m_target = nullptr;
    BoundsHandleAnchor m_anchor;
    moth::ui::FloatVec2 m_position;
    bool m_holding = false;

    virtual bool IsInBounds(moth::ui::IntVec2 const& pos) const = 0;

    bool OnMouseDown(moth::ui::EventMouseDown const& event);
    bool OnMouseUp(moth::ui::EventMouseUp const& event);
    bool OnMouseMove(moth::ui::EventMouseMove const& event);
    virtual void UpdatePosition(moth::ui::IntVec2 const& position) = 0;
};
