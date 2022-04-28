#include "common.h"
#include "bounds_handle.h"
#include "moth_ui/event_dispatch.h"
#include "bounds_widget.h"
#include "panels/editor_panel_canvas.h"

BoundsHandle::BoundsHandle(BoundsWidget& widget, BoundsHandleAnchor const& anchor)
    : m_widget(widget)
    , m_anchor(anchor) {
}

BoundsHandle::~BoundsHandle() {
}

void BoundsHandle::SetTarget(moth_ui::Node* node) {
    m_target = node;
}

bool BoundsHandle::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &BoundsHandle::OnMouseDown);
    dispatch.Dispatch(this, &BoundsHandle::OnMouseUp);
    dispatch.Dispatch(this, &BoundsHandle::OnMouseMove);
    return dispatch.GetHandled();
}

bool BoundsHandle::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (nullptr == m_target) {
        return false;
    }

    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }

    if (IsInBounds(event.GetPosition())) {
        m_holding = true;
        m_widget.BeginEdit();
        return true;
    }

    return false;
}

bool BoundsHandle::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (nullptr == m_target) {
        return false;
    }

    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }

    if (m_holding) {
        m_widget.EndEdit();
    }

    m_holding = false;

    return false;
}

bool BoundsHandle::OnMouseMove(moth_ui::EventMouseMove const& event) {
    if (nullptr == m_target) {
        return false;
    }

    if (m_holding) {
        auto& canvasPanel = m_widget.GetCanvasPanel();
        auto const worldPosition = canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::AppSpace, EditorPanelCanvas::CoordSpace::WorldSpace, int>(event.GetPosition());
        UpdatePosition(m_widget.GetCanvasPanel().SnapToGrid(worldPosition));
    }
    return false;
}
