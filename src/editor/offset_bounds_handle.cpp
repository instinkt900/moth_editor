#include "common.h"
#include "offset_bounds_handle.h"
#include "moth_ui/group.h"
#include "moth_ui/layout/layout_entity.h"
#include "bounds_widget.h"
#include "editor_layer.h"
#include "panels/editor_panel_canvas.h"

OffsetBoundsHandle::OffsetBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
    : BoundsHandle(widget, anchor) {
}

OffsetBoundsHandle::~OffsetBoundsHandle() {
}

void OffsetBoundsHandle::Draw(SDL_Renderer& renderer) {
    if (!m_target) {
        return;
    }

    auto const bounds = static_cast<moth_ui::FloatRect>(m_target->GetScreenRect());
    auto const dimensions = moth_ui::FloatVec2{ bounds.w(), bounds.h() };

    moth_ui::FloatVec2 const anchor{
        m_anchor.Left ? 0.0f : (m_anchor.Right ? 1.0f : 0.5f),
        m_anchor.Top ? 0.0f : (m_anchor.Bottom ? 1.0f : 0.5f)
    };

    m_position = bounds.topLeft + dimensions * anchor;

    auto const handleSize = moth_ui::FloatVec2{ m_size, m_size };
    auto const halfHandleSize = handleSize / 2.0f;

    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto const drawList = ImGui::GetWindowDrawList();
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
