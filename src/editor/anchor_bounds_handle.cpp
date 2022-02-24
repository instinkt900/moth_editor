#include "common.h"
#include "anchor_bounds_handle.h"
#include "uilib/node.h"
#include "uilib/group.h"
#include "uilib/utils/math_utils.h"
#include "bounds_widget.h"
#include "editor_layer.h"

namespace ui {
    AnchorBoundsHandle::AnchorBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
        : BoundsHandle(widget, anchor) {
    }

    AnchorBoundsHandle::~AnchorBoundsHandle() {
    }

    void AnchorBoundsHandle::Draw(SDL_Renderer& renderer) {
        if (!m_target || !m_target->GetParent()) {
            return;
        }

        auto const& parentScreenRect = m_target->GetParent()->GetScreenRect();
        auto const& layoutRect = m_target->GetLayoutRect();

        FloatVec2 const canvasDim{ static_cast<float>(parentScreenRect.bottomRight.x - parentScreenRect.topLeft.x), static_cast<float>(parentScreenRect.bottomRight.y - parentScreenRect.topLeft.y) };
        FloatVec2 const anchor{ m_anchor.Left ? 0.0f : (m_anchor.Right ? 1.0f : 0.5f), m_anchor.Top ? 0.0f : (m_anchor.Bottom ? 1.0f : 0.5f) };
        FloatVec2 const adjust = lerp(layoutRect.anchor.topLeft, layoutRect.anchor.bottomRight, anchor);
        m_position = static_cast<FloatVec2>(parentScreenRect.topLeft) + canvasDim * adjust;

        int const halfSize = m_size / 2;

        float x1 = 0;
        float x2 = 0;
        float y1 = 0;
        float y2 = 0;
        float const offset = halfSize + 3.0f;

        auto const scaleFactor = m_widget.GetEditorLayer().GetScaleFactor();
        auto const scaledPosition = m_position / scaleFactor;

        if (!m_anchor.Top || !m_anchor.Bottom) {
            x1 = scaledPosition.x - offset;
            x2 = scaledPosition.x + offset;
            y1 = scaledPosition.y - offset + static_cast<int>(offset * 2 * anchor.y);
            SDL_RenderDrawLineF(&renderer, x1, y1, x2, y1);
        }

        if (!m_anchor.Left || !m_anchor.Right) {
            y1 = scaledPosition.y - offset;
            y2 = scaledPosition.y + offset;
            x1 = scaledPosition.x - offset + static_cast<int>(offset * 2 * anchor.x);
            SDL_RenderDrawLineF(&renderer, x1, y1, x1, y2);
        }
    }

    bool AnchorBoundsHandle::IsInBounds(IntVec2 const& pos) const {
        auto const scaleFactor = m_widget.GetEditorLayer().GetScaleFactor();

        FloatVec2 const anchor{ m_anchor.Left ? 0.0f : (m_anchor.Right ? 1.0f : 0.5f), m_anchor.Top ? 0.0f : (m_anchor.Bottom ? 1.0f : 0.5f) };

        int const halfSize = static_cast<int>(m_size / 2 * scaleFactor);

        int x1 = 0;
        int x2 = 0;
        int y1 = 0;
        int y2 = 0;
        int const offset = halfSize + 3;

        bool inside = false;

        IntVec2 position = static_cast<IntVec2>(m_position);

        if (!m_anchor.Top || !m_anchor.Bottom) {
            IntRect r1;
            r1.topLeft.x = position.x - offset;
            r1.bottomRight.x = r1.topLeft.x + offset * 2;
            r1.topLeft.y = position.y - offset + static_cast<int>(offset * 2 * anchor.y) - 3;
            r1.bottomRight.y = r1.topLeft.y + 6;
            if (IsInRect(pos, r1)) {
                return true;
            }
        }

        if (!m_anchor.Left || !m_anchor.Right) {
            IntRect r1;
            r1.topLeft.y = position.y - offset;
            r1.bottomRight.y = r1.topLeft.y + offset * 2;
            r1.topLeft.x = position.x - offset + static_cast<int>(offset * 2 * anchor.x) - 3;
            r1.bottomRight.x = r1.topLeft.x + 6;
            if (IsInRect(pos, r1)) {
                return true;
            }
        }

        return false;
    }

    void AnchorBoundsHandle::UpdatePosition(IntVec2 const& position) {
        auto const parent = m_target->GetParent();
        auto const& parentRect = parent->GetScreenRect();

        FloatVec2 const parentOffset = static_cast<FloatVec2>(parentRect.topLeft);
        FloatVec2 const parentDimensions = static_cast<FloatVec2>(parentRect.bottomRight - parentRect.topLeft);

        auto& bounds = m_target->GetLayoutRect();

        FloatVec2 const mousePosition = static_cast<FloatVec2>(position);
        FloatVec2 const newAnchorPos = mousePosition - parentOffset;
        FloatVec2 const newAnchor = newAnchorPos / parentDimensions;

        FloatVec2 const topLeftAnchorDelta = newAnchor - bounds.anchor.topLeft;
        FloatVec2 const bottomRightOffsetDelta = newAnchor - bounds.anchor.bottomRight;

        bounds.anchor.topLeft += topLeftAnchorDelta * FloatVec2{ static_cast<float>(m_anchor.Left), static_cast<float>(m_anchor.Top) };
        bounds.anchor.bottomRight += bottomRightOffsetDelta * FloatVec2{ static_cast<float>(m_anchor.Right), static_cast<float>(m_anchor.Bottom) };

        m_target->RecalculateBounds();
    }
}
