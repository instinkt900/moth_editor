#include "common.h"
#include "offset_bounds_handle.h"
#include "moth_ui/nodes/group.h"
#include "moth_ui/layout/layout_entity.h"
#include "moth_ui/utils/transform.h"
#include "bounds_widget.h"
#include "editor_layer.h"
#include "panels/editor_panel_canvas.h"

#include <cmath>

OffsetBoundsHandle::OffsetBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
    : BoundsHandle(widget, anchor) {
}

OffsetBoundsHandle::~OffsetBoundsHandle() {
}

void OffsetBoundsHandle::Draw() {
    if (m_target == nullptr) {
        return;
    }

    if (!m_holding) {
        m_dragActive = false;
    }

    m_position = m_widget.GetNodeAnchorWorldPos(m_anchor);

    auto const handleSize = moth_ui::FloatVec2{ m_size, m_size };
    auto const halfHandleSize = handleSize / 2.0f;

    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto* const drawList = ImGui::GetWindowDrawList();
    auto const drawPosition = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    auto const color = moth_ui::ToABGR(canvasPanel.GetEditorLayer().GetConfig().SelectionColor);
    drawList->AddRectFilled(ImVec2{ drawPosition.x - halfHandleSize.x, drawPosition.y - halfHandleSize.y }, ImVec2{ drawPosition.x + halfHandleSize.x, drawPosition.y + halfHandleSize.y }, color);
}

bool OffsetBoundsHandle::IsInBounds(moth_ui::IntVec2 const& pos) const {
    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto const drawPosition = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    auto const halfSize = static_cast<int>(m_size / 2);

    moth_ui::IntRect r;
    r.topLeft = static_cast<moth_ui::IntVec2>(drawPosition - halfSize);
    r.bottomRight = static_cast<moth_ui::IntVec2>(drawPosition + halfSize);
    return IsInRect(pos, r);
}

void OffsetBoundsHandle::UpdatePosition(moth_ui::IntVec2 const& position) {
    auto const currentPos = static_cast<moth_ui::FloatVec2>(m_widget.GetCanvasPanel().SnapToGrid(position));

    if (!m_dragActive) {
        m_prevDragWorldPos = currentPos;
        m_dragActive = true;
        return;
    }

    auto const delta = currentPos - m_prevDragWorldPos;
    m_prevDragWorldPos = currentPos;

    // Project the world-space delta onto the node's unrotated X and Y axes so that
    // the offset values (which live in the axis-aligned parent coordinate space) are
    // updated correctly regardless of the node's current rotation.
    float const rotation = m_target->GetRotation() * moth_ui::kDegToRad;
    float const c = std::cos(rotation);
    float const s = std::sin(rotation);
    float const dx = (delta.x * c) + (delta.y * s);
    float const dy = (-delta.x * s) + (delta.y * c);

    auto& bounds = m_target->GetLayoutRect();
    bounds.offset.topLeft.x += dx * static_cast<float>(m_anchor.Left);
    bounds.offset.topLeft.y += dy * static_cast<float>(m_anchor.Top);
    bounds.offset.bottomRight.x += dx * static_cast<float>(m_anchor.Right);
    bounds.offset.bottomRight.y += dy * static_cast<float>(m_anchor.Bottom);

    m_target->RecalculateBounds();
}
