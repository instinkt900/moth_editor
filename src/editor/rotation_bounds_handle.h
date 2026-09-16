#pragma once

#include "bounds_handle.h"

class RotationBoundsHandle : public BoundsHandle {
public:
    RotationBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    ~RotationBoundsHandle() override;

    bool OnEvent(moth::ui::Event const& event) override;
    void Draw() override;

private:
    static constexpr float m_size = 10.0f;
    static constexpr float m_offset = 8.0f;

    float m_startAngle = 0.0f;
    float m_originalRotation = 0.0f;

    bool IsInBounds(moth::ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth::ui::IntVec2 const& position) override;

    moth::ui::FloatVec2 GetPivotWorldPos() const;

    bool OnMouseDown(moth::ui::EventMouseDown const& event);
    bool OnMouseUp(moth::ui::EventMouseUp const& event);
    bool OnMouseMove(moth::ui::EventMouseMove const& event);
};
