#include "common.h"
#include "anchor_bounds_handle.h"
#include "moth_ui/node.h"
#include "moth_ui/group.h"
#include "moth_ui/utils/math_utils.h"
#include "bounds_widget.h"
#include "editor_layer.h"
#include "moth_ui/utils/interp.h"
#include "panels/editor_panel_canvas.h"

AnchorBoundsHandle::AnchorBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
    : BoundsHandle(widget, anchor) {
}

AnchorBoundsHandle::~AnchorBoundsHandle() {
}

void AnchorBoundsHandle::Draw(SDL_Renderer& renderer) {
    if (!m_target || !m_target->GetParent()) {
        return;
    }

    auto const& layoutRect = m_target->GetLayoutRect();

    auto const parentBounds = static_cast<moth_ui::FloatRect>(m_target->GetParent()->GetScreenRect());
    auto const parentDimensions = moth_ui::FloatVec2{ parentBounds.w(), parentBounds.h() };

    moth_ui::FloatVec2 const anchor{
        m_anchor.Left ? 0.0f : (m_anchor.Right ? 1.0f : 0.5f),
        m_anchor.Top ? 0.0f : (m_anchor.Bottom ? 1.0f : 0.5f)
    };

    auto const adjust = lerp(layoutRect.anchor.topLeft, layoutRect.anchor.bottomRight, anchor);
    m_position = parentBounds.topLeft + parentDimensions * adjust;

    int const halfSize = m_size / 2;

    float x1 = 0;
    float x2 = 0;
    float y1 = 0;
    float y2 = 0;
    float const offset = halfSize + 3.0f;

    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto const drawList = ImGui::GetWindowDrawList();
    auto const drawPosition = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    if (!m_anchor.Top || !m_anchor.Bottom) {
        x1 = drawPosition.x - offset;
        x2 = drawPosition.x + offset;
        y1 = drawPosition.y - offset + static_cast<int>(offset * 2 * anchor.y);
        drawList->AddLine(ImVec2{ x1, y1 }, ImVec2{ x2, y1 }, 0xFFFF0000);
    }

    if (!m_anchor.Left || !m_anchor.Right) {
        y1 = drawPosition.y - offset;
        y2 = drawPosition.y + offset;
        x1 = drawPosition.x - offset + static_cast<int>(offset * 2 * anchor.x);
        drawList->AddLine(ImVec2{ x1, y1 }, ImVec2{ x1, y2 }, 0xFFFF0000);
    }
}

bool AnchorBoundsHandle::IsInBounds(moth_ui::IntVec2 const& pos) const {
    moth_ui::FloatVec2 const anchor{
        m_anchor.Left ? 0.0f : (m_anchor.Right ? 1.0f : 0.5f),
        m_anchor.Top ? 0.0f : (m_anchor.Bottom ? 1.0f : 0.5f)
    };

    int const halfSize = m_size / 2;

    int x1 = 0;
    int x2 = 0;
    int y1 = 0;
    int y2 = 0;
    int const offset = halfSize + 3;

    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto const drawPosition = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, int>(m_position);

    bool inside = false;

    if (!m_anchor.Top || !m_anchor.Bottom) {
        moth_ui::IntRect r1;
        r1.topLeft.x = drawPosition.x - offset;
        r1.bottomRight.x = r1.topLeft.x + offset * 2;
        r1.topLeft.y = drawPosition.y - offset + static_cast<int>(offset * 2 * anchor.y) - 3;
        r1.bottomRight.y = r1.topLeft.y + 6;
        if (moth_ui::IsInRect(pos, r1)) {
            return true;
        }
    }

    if (!m_anchor.Left || !m_anchor.Right) {
        moth_ui::IntRect r1;
        r1.topLeft.y = drawPosition.y - offset;
        r1.bottomRight.y = r1.topLeft.y + offset * 2;
        r1.topLeft.x = drawPosition.x - offset + static_cast<int>(offset * 2 * anchor.x) - 3;
        r1.bottomRight.x = r1.topLeft.x + 6;
        if (moth_ui::IsInRect(pos, r1)) {
            return true;
        }
    }

    return false;
}

void AnchorBoundsHandle::UpdatePosition(moth_ui::IntVec2 const& position) {
    auto const parent = m_target->GetParent();
    auto const& parentRect = parent->GetScreenRect();

    auto const parentOffset = static_cast<moth_ui::FloatVec2>(parentRect.topLeft);
    auto const parentDimensions = static_cast<moth_ui::FloatVec2>(parentRect.bottomRight - parentRect.topLeft);

    auto& bounds = m_target->GetLayoutRect();

    auto const mousePosition = static_cast<moth_ui::FloatVec2>(position);
    auto const newAnchorPos = mousePosition - parentOffset;
    auto const newAnchor = newAnchorPos / parentDimensions;

    auto const topLeftAnchorDelta = newAnchor - bounds.anchor.topLeft;
    auto const bottomRightOffsetDelta = newAnchor - bounds.anchor.bottomRight;

    bounds.anchor.topLeft += topLeftAnchorDelta * moth_ui::FloatVec2{ static_cast<float>(m_anchor.Left), static_cast<float>(m_anchor.Top) };
    bounds.anchor.bottomRight += bottomRightOffsetDelta * moth_ui::FloatVec2{ static_cast<float>(m_anchor.Right), static_cast<float>(m_anchor.Bottom) };

    auto const& screenRect = m_target->GetScreenRect();
    bounds.offset.topLeft.x = screenRect.topLeft.x - (parentRect.w() * bounds.anchor.topLeft.x) - parentRect.topLeft.x;
    bounds.offset.topLeft.y = screenRect.topLeft.y - (parentRect.h() * bounds.anchor.topLeft.y) - parentRect.topLeft.y;
    bounds.offset.bottomRight.x = screenRect.bottomRight.x - (parentRect.w() * bounds.anchor.bottomRight.x) - parentRect.topLeft.x;
    bounds.offset.bottomRight.y = screenRect.bottomRight.y - (parentRect.h() * bounds.anchor.bottomRight.y) - parentRect.topLeft.y;

    m_target->RecalculateBounds();
}
