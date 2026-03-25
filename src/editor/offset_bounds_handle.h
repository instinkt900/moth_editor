#pragma once

#include "bounds_handle.h"

class OffsetBoundsHandle : public BoundsHandle {
public:
    OffsetBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    ~OffsetBoundsHandle() override;

    void Draw() override;

private:
    float m_size = 12.0f;
    moth_ui::FloatVec2 m_prevDragWorldPos;
    moth_ui::FloatVec2 m_startPivotWorld;
    bool m_dragActive = false;

    bool IsInBounds(moth_ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth_ui::IntVec2 const& position) override;
};
