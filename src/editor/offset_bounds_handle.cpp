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
    auto const currentPos = static_cast<moth_ui::FloatVec2>(position);

    // Capture start position and pivot world pos for the first update of each drag.
    if (!m_dragActive) {
        m_prevDragWorldPos = currentPos;
        m_dragActive = true;
        auto const bounds = static_cast<moth_ui::FloatRect>(m_target->GetScreenRect());
        auto const dims = moth_ui::FloatVec2{ bounds.w(), bounds.h() };
        auto const entity = m_target->GetLayoutEntity();
        auto const pivotFrac = entity ? entity->m_pivot : moth_ui::FloatVec2{ 0.5f, 0.5f };
        m_startPivotWorld = bounds.topLeft + dims * pivotFrac;
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

    // Keep the pivot at the same world position it was at when the drag started,
    // so the rotation anchor doesn't drift as the node is resized.
    auto const newBounds = static_cast<moth_ui::FloatRect>(m_target->GetScreenRect());
    auto const newDims = moth_ui::FloatVec2{ newBounds.w(), newBounds.h() };
    if (newDims.x != 0.0f && newDims.y != 0.0f) {
        auto const entity = m_target->GetLayoutEntity();
        if (entity) {
            auto const newPivot = (m_startPivotWorld - newBounds.topLeft) / newDims;
            entity->m_pivot = newPivot;
            m_target->SetPivot(newPivot);
        }
    }
}
