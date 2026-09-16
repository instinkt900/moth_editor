#pragma once

#include "bounds_handle.h"

class AnchorBoundsHandle : public BoundsHandle {
public:
    AnchorBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    ~AnchorBoundsHandle() override;

    void Draw() override;

private:
    int m_size = 12;

    bool IsInBounds(moth::ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth::ui::IntVec2 const& position) override;
};
