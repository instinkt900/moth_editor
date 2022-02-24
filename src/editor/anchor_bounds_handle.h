#pragma once

#include "bounds_handle.h"

namespace ui {
    class AnchorBoundsHandle : public BoundsHandle {
    public:
        AnchorBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor);
        virtual ~AnchorBoundsHandle();

        void Draw(SDL_Renderer& renderer) override;

    private:
        int m_size = 10;

        bool IsInBounds(IntVec2 const& pos) const override;
        void UpdatePosition(IntVec2 const& position) override;
    };
}
