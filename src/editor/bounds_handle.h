#pragma once

#include "moth_ui/event_listener.h"
#include "moth_ui/events/event_mouse.h"

class BoundsWidget;

struct BoundsHandleAnchor {
    int Top = 0;
    int Left = 0;
    int Bottom = 0;
    int Right = 0;
};

class BoundsHandle : public moth_ui::EventListener {
public:
    BoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    virtual ~BoundsHandle();

    virtual void SetTarget(moth_ui::Node* node);

    virtual bool OnEvent(moth_ui::Event const& event) override;
    virtual void Draw() = 0;

    static BoundsHandleAnchor constexpr TopLeft{ 1, 1, 0, 0 };
    static BoundsHandleAnchor constexpr TopRight{ 1, 0, 0, 1 };
    static BoundsHandleAnchor constexpr BottomLeft{ 0, 1, 1, 0 };
    static BoundsHandleAnchor constexpr BottomRight{ 0, 0, 1, 1 };
    static BoundsHandleAnchor constexpr Top{ 1, 0, 0, 0 };
    static BoundsHandleAnchor constexpr Left{ 0, 1, 0, 0 };
    static BoundsHandleAnchor constexpr Bottom{ 0, 0, 1, 0 };
    static BoundsHandleAnchor constexpr Right{ 0, 0, 0, 1 };

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
