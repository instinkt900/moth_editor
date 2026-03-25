#include "common.h"
#include "pivot_bounds_handle.h"
#include "bounds_widget.h"
#include "editor_layer.h"
#include "panels/editor_panel_canvas.h"
#include "actions/editor_action.h"
#include "moth_ui/events/event_dispatch.h"
#include "moth_ui/layout/layout_entity.h"
#include "moth_ui/nodes/node.h"
#include "moth_ui/utils/transform.h"

#include <cmath>

PivotBoundsHandle::PivotBoundsHandle(BoundsWidget& widget)
    : BoundsHandle(widget, {}) {
}

PivotBoundsHandle::~PivotBoundsHandle() {
}

bool PivotBoundsHandle::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &PivotBoundsHandle::OnMouseDown);
    dispatch.Dispatch(this, &PivotBoundsHandle::OnMouseUp);
    dispatch.Dispatch(this, &PivotBoundsHandle::OnMouseMove);
    return dispatch.GetHandled();
}

void PivotBoundsHandle::Draw() {
    if (m_target == nullptr) {
        return;
    }

    auto const& screenRect = m_target->GetScreenRect();
    auto const bounds = static_cast<moth_ui::FloatRect>(screenRect);
    auto const dims = moth_ui::FloatVec2{ bounds.w(), bounds.h() };
    auto const entity = m_target->GetLayoutEntity();
    auto const pivot = entity ? entity->m_pivot : moth_ui::FloatVec2{ 0.5f, 0.5f };

    m_position = bounds.topLeft + dims * pivot;

    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto* const drawList = ImGui::GetWindowDrawList();
    auto const drawPos = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    auto const color = moth_ui::ToABGR(canvasPanel.GetEditorLayer().GetConfig().SelectionColor);

    drawList->AddCircle(ImVec2{ drawPos.x, drawPos.y }, m_radius, color, 0, 2.0f);
    drawList->AddLine(ImVec2{ drawPos.x - m_radius, drawPos.y }, ImVec2{ drawPos.x + m_radius, drawPos.y }, color, 1.0f);
    drawList->AddLine(ImVec2{ drawPos.x, drawPos.y - m_radius }, ImVec2{ drawPos.x, drawPos.y + m_radius }, color, 1.0f);
}

bool PivotBoundsHandle::IsInBounds(moth_ui::IntVec2 const& pos) const {
    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto const drawPos = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    auto const dx = static_cast<float>(pos.x) - drawPos.x;
    auto const dy = static_cast<float>(pos.y) - drawPos.y;
    return ((dx * dx) + (dy * dy)) <= (m_radius * m_radius);
}

void PivotBoundsHandle::UpdatePosition(moth_ui::IntVec2 const& position) {
    auto const& screenRect = m_target->GetScreenRect();
    auto const bounds = static_cast<moth_ui::FloatRect>(screenRect);
    auto const dims = moth_ui::FloatVec2{ bounds.w(), bounds.h() };

    if (dims.x == 0.0f || dims.y == 0.0f) {
        return;
    }

    auto const entity = m_target->GetLayoutEntity();
    if (!entity) {
        return;
    }

    float const rotation = m_target->GetRotation() * moth_ui::kDegToRad;
    float const c = std::cos(rotation);
    float const s = std::sin(rotation);

    // Current pivot in world space (the rotation centre — unchanged by rotation).
    auto const pivotLocalOld = entity->m_pivot * dims;
    auto const pivotWorldOld = bounds.topLeft + pivotLocalOld;

    // Back-rotate the mouse-from-pivot vector into the node's local (unrotated)
    // space to get how far the pivot should move in local coordinates.
    auto const mousePos = static_cast<moth_ui::FloatVec2>(position);
    auto const mouseFromPivot = mousePos - pivotWorldOld;
    auto const deltaLocal = moth_ui::FloatVec2{
        (c * mouseFromPivot.x) + (s * mouseFromPivot.y),
        (-s * mouseFromPivot.x) + (c * mouseFromPivot.y)
    };

    // Clamp the new pivot to [0,1] bounds.
    auto newPivotLocal = pivotLocalOld + deltaLocal;
    newPivotLocal.x = std::clamp(newPivotLocal.x, 0.0f, dims.x);
    newPivotLocal.y = std::clamp(newPivotLocal.y, 0.0f, dims.y);
    auto const newPivot = newPivotLocal / dims;

    // Compute the translation needed to keep all corners visually fixed.
    // Derivation: for each corner C, world position = TL + R*(C - pivotLocal).
    // Solving for new TL such that corners are unchanged gives:
    //   deltaTranslation = (R(θ) - I) * actualDeltaLocal
    auto const actualDelta = newPivotLocal - pivotLocalOld;
    auto const deltaTranslation = moth_ui::FloatVec2{
        ((c - 1.0f) * actualDelta.x) - (s * actualDelta.y),
        (s * actualDelta.x) + ((c - 1.0f) * actualDelta.y)
    };

    auto& layoutRect = m_target->GetLayoutRect();
    layoutRect.offset.topLeft.x += deltaTranslation.x;
    layoutRect.offset.topLeft.y += deltaTranslation.y;
    layoutRect.offset.bottomRight.x += deltaTranslation.x;
    layoutRect.offset.bottomRight.y += deltaTranslation.y;

    entity->m_pivot = newPivot;
    m_target->SetPivot(newPivot);
    m_target->RecalculateBounds();
}

bool PivotBoundsHandle::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (m_target == nullptr) {
        return false;
    }
    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }
    if (IsInBounds(event.GetPosition())) {
        auto const entity = m_target->GetLayoutEntity();
        m_originalPivot = entity ? entity->m_pivot : moth_ui::FloatVec2{ 0.5f, 0.5f };
        m_originalOffset = m_target->GetLayoutRect().offset;
        m_holding = true;
        return true;
    }
    return false;
}

bool PivotBoundsHandle::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (m_target == nullptr) {
        return false;
    }
    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }
    if (m_holding) {
        auto const entity = m_target->GetLayoutEntity();
        if (entity && entity->m_pivot != m_originalPivot) {
            auto node = m_widget.GetSelection();
            auto const finalPivot = entity->m_pivot;
            auto const finalOffset = m_target->GetLayoutRect().offset;
            auto const originalPivot = m_originalPivot;
            auto const originalOffset = m_originalOffset;

            auto action = std::make_unique<BasicAction>(
                [entity, node, finalPivot, finalOffset]() {
                    entity->m_pivot = finalPivot;
                    node->GetLayoutRect().offset = finalOffset;
                    node->SetPivot(finalPivot);
                    node->RecalculateBounds();
                },
                [entity, node, originalPivot, originalOffset]() {
                    entity->m_pivot = originalPivot;
                    node->GetLayoutRect().offset = originalOffset;
                    node->SetPivot(originalPivot);
                    node->RecalculateBounds();
                }
            );
            m_widget.GetCanvasPanel().GetEditorLayer().PerformEditAction(std::move(action));
        }
    }
    m_holding = false;
    return false;
}

bool PivotBoundsHandle::OnMouseMove(moth_ui::EventMouseMove const& event) {
    if (m_target == nullptr) {
        return false;
    }
    if (m_holding) {
        auto& canvasPanel = m_widget.GetCanvasPanel();
        auto const worldPosition = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::AppSpace, EditorPanelCanvas::CoordSpace::WorldSpace, int>(event.GetPosition());
        UpdatePosition(worldPosition);
    }
    return false;
}
