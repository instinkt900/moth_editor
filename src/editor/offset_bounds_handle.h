#pragma once

#include "bounds_handle.h"

class OffsetBoundsHandle : public BoundsHandle {
public:
    OffsetBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    virtual ~OffsetBoundsHandle();

    void Draw(SDL_Renderer& renderer) override;

private:
    float m_size = 10.0f;

    bool IsInBounds(moth_ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth_ui::IntVec2 const& position) override;
};
