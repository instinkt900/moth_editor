#include "common.h"
#include "rotation_bounds_handle.h"
#include "bounds_widget.h"
#include "editor_layer.h"
#include "panels/editor_panel_canvas.h"
#include "moth_ui/events/event_dispatch.h"
#include "moth_ui/nodes/node.h"
#include "moth_ui/layout/layout_entity.h"
#include "moth_ui/utils/transform.h"

#include <cmath>

RotationBoundsHandle::RotationBoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
    : BoundsHandle(widget, anchor) {
}

RotationBoundsHandle::~RotationBoundsHandle() {
}

bool RotationBoundsHandle::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &RotationBoundsHandle::OnMouseDown);
    dispatch.Dispatch(this, &RotationBoundsHandle::OnMouseUp);
    dispatch.Dispatch(this, &RotationBoundsHandle::OnMouseMove);
    return dispatch.GetHandled();
}

moth_ui::FloatVec2 RotationBoundsHandle::GetPivotWorldPos() const {
    auto const& screenRect = m_target->GetScreenRect();
    auto const bounds = static_cast<moth_ui::FloatRect>(screenRect);
    auto const dims = moth_ui::FloatVec2{ bounds.w(), bounds.h() };
    auto const entity = m_target->GetLayoutEntity();
    auto const pivot = entity ? entity->m_pivot : moth_ui::FloatVec2{ 0.5f, 0.5f };
    return bounds.topLeft + dims * pivot;
}

void RotationBoundsHandle::Draw() {
    if (m_target == nullptr) {
        return;
    }

    float anchorX = 0.5f;
    if (m_anchor.Left) { anchorX = 0.0f; }
    else if (m_anchor.Right) { anchorX = 1.0f; }
    float anchorY = 0.5f;
    if (m_anchor.Top) { anchorY = 0.0f; }
    else if (m_anchor.Bottom) { anchorY = 1.0f; }

    float const localDirX = (anchorX == 0.0f) ? -1.0f : 1.0f;
    float const localDirY = (anchorY == 0.0f) ? -1.0f : 1.0f;

    float const rotation = m_target->GetRotation() * moth_ui::kDegToRad;
    float const c = std::cos(rotation);
    float const s = std::sin(rotation);
    moth_ui::FloatVec2 const rotatedDir = {
        (c * localDirX) - (s * localDirY),
        (s * localDirX) + (c * localDirY)
    };

    moth_ui::FloatVec2 const cornerWorld = m_widget.GetNodeAnchorWorldPos(m_anchor);
    m_position = cornerWorld + rotatedDir * m_offset;

    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto* const drawList = ImGui::GetWindowDrawList();
    auto const drawPos = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    auto const color = moth_ui::ToABGR(canvasPanel.GetEditorLayer().GetConfig().SelectionColor);
    auto const halfSize = m_size / 2.0f;
    drawList->AddCircleFilled(ImVec2{ drawPos.x, drawPos.y }, halfSize, color);
}

bool RotationBoundsHandle::IsInBounds(moth_ui::IntVec2 const& pos) const {
    auto& canvasPanel = m_widget.GetCanvasPanel();
    auto const drawPos = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace>(m_position);
    auto const dx = static_cast<float>(pos.x) - drawPos.x;
    auto const dy = static_cast<float>(pos.y) - drawPos.y;
    auto const halfSize = m_size / 2.0f;
    return (dx * dx + dy * dy) <= (halfSize * halfSize);
}

void RotationBoundsHandle::UpdatePosition(moth_ui::IntVec2 const& position) {
    auto const pivotWorld = GetPivotWorldPos();
    auto const mousePos = static_cast<moth_ui::FloatVec2>(position);
    auto const dx = mousePos.x - pivotWorld.x;
    auto const dy = mousePos.y - pivotWorld.y;
    float const currentAngle = std::atan2(dy, dx);
    float deltaAngle = currentAngle - m_startAngle;
    // Normalise into (-π, π] to avoid a ±2π jump when crossing the atan2 seam.
    constexpr float kPi = 3.14159265358979f;
    while (deltaAngle >  kPi) { deltaAngle -= 2.0f * kPi; }
    while (deltaAngle < -kPi) { deltaAngle += 2.0f * kPi; }
    float newRotation = m_originalRotation + (deltaAngle * moth_ui::kRadToDeg);
    auto const& config = m_widget.GetCanvasPanel().GetEditorLayer().GetConfig();
    if (config.SnapToAngle && config.SnapAngle > 0.0f) {
        newRotation = std::round(newRotation / config.SnapAngle) * config.SnapAngle;
    }
    m_target->SetRotation(newRotation);
}

bool RotationBoundsHandle::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (m_target == nullptr) {
        return false;
    }
    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }
    if (IsInBounds(event.GetPosition())) {
        auto& canvasPanel = m_widget.GetCanvasPanel();
        auto const worldPos = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::AppSpace, EditorPanelCanvas::CoordSpace::WorldSpace, int>(event.GetPosition());
        auto const pivotWorld = GetPivotWorldPos();
        auto const mousePos = static_cast<moth_ui::FloatVec2>(worldPos);
        m_startAngle = std::atan2(mousePos.y - pivotWorld.y, mousePos.x - pivotWorld.x);
        m_originalRotation = m_target->GetRotation();
        m_holding = true;
        m_widget.GetCanvasPanel().GetEditorLayer().BeginEditRotation(m_widget.GetSelection());
        return true;
    }
    return false;
}

bool RotationBoundsHandle::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (m_target == nullptr) {
        return false;
    }
    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }
    if (m_holding) {
        m_widget.GetCanvasPanel().GetEditorLayer().EndEditRotation();
        m_holding = false;
        return true;
    }
    m_holding = false;
    return false;
}

bool RotationBoundsHandle::OnMouseMove(moth_ui::EventMouseMove const& event) {
    if (m_target == nullptr) {
        return false;
    }
    if (m_holding) {
        auto& canvasPanel = m_widget.GetCanvasPanel();
        auto const worldPos = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::AppSpace, EditorPanelCanvas::CoordSpace::WorldSpace, int>(event.GetPosition());
        UpdatePosition(worldPos);
        return true;
    }
    return false;
}
