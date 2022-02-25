#pragma once

#include "bounds_handle.h"

class AnchorBoundsHandle : public BoundsHandle {
public:
    AnchorBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
    virtual ~AnchorBoundsHandle();

    void Draw(SDL_Renderer& renderer) override;

private:
    int m_size = 10;

    bool IsInBounds(moth_ui::IntVec2 const& pos) const override;
    void UpdatePosition(moth_ui::IntVec2 const& position) override;
};
