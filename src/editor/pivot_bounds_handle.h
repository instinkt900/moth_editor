#pragma once

#include "bounds_handle.h"
#include "moth_ui/utils/rect.h"

class PivotBoundsHandle : public BoundsHandle {
public:
    PivotBoundsHandle(BoundsWidget& widget);
    ~PivotBoundsHandle() override;

    bool OnEvent(moth_ui::Event const& event) override;
    void Draw() override;

private:
    static constexpr float m_radius = 6.0f;

    moth_ui::FloatVec2 m_originalPivot;
    moth_ui::FloatRect m_originalOffset; ///< Layout rect offset at drag start

    bool IsInBounds(moth_ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth_ui::IntVec2 const& position) override;

    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
    bool OnMouseMove(moth_ui::EventMouseMove const& event);
};
