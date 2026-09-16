#pragma once

#include "bounds_handle.h"

class PivotBoundsHandle : public BoundsHandle {
public:
    PivotBoundsHandle(BoundsWidget& widget);
    ~PivotBoundsHandle() override;

    bool OnEvent(moth::ui::Event const& event) override;
    void Draw() override;

private:
    static constexpr float m_radius = 6.0f;


    bool IsInBounds(moth::ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth::ui::IntVec2 const& position) override;

    bool OnMouseDown(moth::ui::EventMouseDown const& event);
    bool OnMouseUp(moth::ui::EventMouseUp const& event);
    bool OnMouseMove(moth::ui::EventMouseMove const& event);
};
