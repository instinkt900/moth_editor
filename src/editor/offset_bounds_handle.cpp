#include "common.h"
#include "offset_bounds_handle.h"
#include "moth_ui/node.h"
#include "moth_ui/layout/layout_entity.h"
#include "bounds_widget.h"
#include "editor_layer.h"

OffsetBoundsHandle::OffsetBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
    : BoundsHandle(widget, anchor) {
}

OffsetBoundsHandle::~OffsetBoundsHandle() {
}

void OffsetBoundsHandle::Draw(SDL_Renderer& renderer) {
    if (!m_target) {
        return;
    }

    auto const scaleFactor = m_widget.GetEditorLayer().GetScaleFactor();

    auto const& bounds = m_target->GetScreenRect();

    auto const dim = static_cast<moth_ui::FloatVec2>(bounds.bottomRight - bounds.topLeft);
    moth_ui::FloatVec2 const anchor = { m_anchor.Left ? 0.0f : (m_anchor.Right ? 1.0f : 0.5f), m_anchor.Top ? 0.0f : (m_anchor.Bottom ? 1.0f : 0.5f) };
    m_position = static_cast<moth_ui::FloatVec2>(bounds.topLeft) + dim * anchor;

    auto const handleSize = moth_ui::FloatVec2{ m_size, m_size };
    auto const halfHandleSize = handleSize / 2.0f;

    auto const offset = (m_position / scaleFactor) - halfHandleSize;

    SDL_FRect const r{ offset.x, offset.y, handleSize.x, handleSize.y };
    SDL_RenderFillRectF(&renderer, &r);
}

bool OffsetBoundsHandle::IsInBounds(moth_ui::IntVec2 const& pos) const {
    auto const scaleFactor = m_widget.GetEditorLayer().GetScaleFactor();
    int const halfSize = static_cast<int>(m_size / 2 * scaleFactor);
    moth_ui::IntRect r;
    r.topLeft = static_cast<moth_ui::IntVec2>(m_position - halfSize);
    r.bottomRight = static_cast<moth_ui::IntVec2>(m_position + halfSize);
    return IsInRect(pos, r);
}

void OffsetBoundsHandle::UpdatePosition(moth_ui::IntVec2 const& position) {
    auto const parent = m_target->GetParent();
    auto const& parentRect = parent->GetScreenRect();

    auto const parentOffset = static_cast<moth_ui::FloatVec2>(parentRect.topLeft);
    auto const parentDimensions = static_cast<moth_ui::FloatVec2>(parentRect.bottomRight - parentRect.topLeft);

    auto& bounds = m_target->GetLayoutRect();

    auto const topLeftAnchorPos = parentOffset + parentDimensions * bounds.anchor.topLeft;
    auto const bottomRightAnchorPos = parentOffset + parentDimensions * bounds.anchor.bottomRight;

    auto const mousePosition = static_cast<moth_ui::FloatVec2>(position);
    auto const topLeftNewOffset = mousePosition - topLeftAnchorPos;
    auto const bottomRightNewOffset = mousePosition - bottomRightAnchorPos;

    auto const topLeftOffsetDelta = topLeftNewOffset - bounds.offset.topLeft;
    auto const bottomRightOffsetDelta = bottomRightNewOffset - bounds.offset.bottomRight;

    bounds.offset.topLeft += topLeftOffsetDelta * moth_ui::FloatVec2{ static_cast<float>(m_anchor.Left), static_cast<float>(m_anchor.Top) };
    bounds.offset.bottomRight += bottomRightOffsetDelta * moth_ui::FloatVec2{ static_cast<float>(m_anchor.Right), static_cast<float>(m_anchor.Bottom) };

    m_target->RecalculateBounds();
}
