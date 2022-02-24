#include "common.h"
#include "bounds_widget.h"
#include "anchor_bounds_handle.h"
#include "offset_bounds_handle.h"
#include "uilib/event_dispatch.h"
#include "editor_layer.h"

namespace ui {
    BoundsWidget::BoundsWidget(EditorLayer& editorLayer)
        : m_editorLayer(editorLayer) {
        // anchor manipulators
        m_handles[0] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::TopLeft);
        m_handles[1] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::TopRight);
        m_handles[2] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::BottomLeft);
        m_handles[3] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::BottomRight);
        m_handles[4] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::Top);
        m_handles[5] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::Left);
        m_handles[6] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::Right);
        m_handles[7] = std::make_unique<AnchorBoundsHandle>(*this, BoundsHandle::Bottom);

        // offset manipulators
        m_handles[8] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::TopLeft);
        m_handles[9] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::TopRight);
        m_handles[10] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::BottomLeft);
        m_handles[11] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::BottomRight);
        m_handles[12] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::Top);
        m_handles[13] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::Left);
        m_handles[14] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::Right);
        m_handles[15] = std::make_unique<OffsetBoundsHandle>(*this, BoundsHandle::Bottom);
    }

    BoundsWidget::~BoundsWidget() {
    }

    bool BoundsWidget::OnEvent(Event const& event) {
        EventDispatch dispatch(event);
        for (auto&& handle : m_handles) {
            dispatch.Dispatch(handle.get());
        }
        dispatch.Dispatch(this, &BoundsWidget::OnMouseDown);
        dispatch.Dispatch(this, &BoundsWidget::OnMouseUp);
        dispatch.Dispatch(this, &BoundsWidget::OnMouseMove);
        return dispatch.GetHandled();
    }

    void BoundsWidget::BeginEdit() {
        m_editorLayer.BeginEditBounds();
    }

    void BoundsWidget::EndEdit() {
        m_editorLayer.EndEditBounds();
    }

    void BoundsWidget::Draw(SDL_Renderer& renderer) {
        auto const selection = m_editorLayer.GetSelection();
        if (selection && selection->IsVisible() && selection->GetParent()) {
            for (auto&& handle : m_handles) {
                handle->SetTarget(selection.get());
            }

            SDL_SetRenderDrawColor(&renderer, 0x00, 0x00, 0xFF, 0xFF);
            auto const& screenRect = selection->GetScreenRect();
            auto const scaleFactor = m_editorLayer.GetScaleFactor();
            auto const pos = screenRect.topLeft / scaleFactor;
            auto const size = (screenRect.bottomRight - screenRect.topLeft) / scaleFactor;
            SDL_Rect const rect{ static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(size.x), static_cast<int>(size.y) };
            SDL_RenderDrawRect(&renderer, &rect);
            for (auto& handle : m_handles) {
                handle->Draw(renderer);
            }
        }
    }

    bool BoundsWidget::OnMouseDown(EventMouseDown const& event) {
        if (event.GetButton() != MouseButton::Left) {
            return false;
        }

        auto selection = m_editorLayer.GetSelection();

        // if we clicked outside the bounds of the selection check for a new selection
        if (!selection || !selection->IsVisible() || !selection->IsInBounds(event.GetPosition())) {
            auto const oldSelection = selection;
            selection = nullptr;

            for (auto&& child : m_editorLayer.GetRoot()->GetChildren()) {
                if (child->IsVisible() && child->IsInBounds(event.GetPosition())) {
                    selection = child;
                    break;
                }
            }

            m_editorLayer.SetSelection(selection);
        }

        if (selection) {
            m_holding = true;
            auto const& canvasTopLeft = m_editorLayer.GetCanvasTopLeft();
            m_grabPosition = SnapToGrid(event.GetPosition() - canvasTopLeft);
            BeginEdit();
            return true;
        }
        return false;
    }

    IntVec2 BoundsWidget::SnapToGrid(IntVec2 const& original) {
        if (m_editorLayer.SnapToGrid()) {
            int const spacing = m_editorLayer.GetGridSpacing();
            float const s = static_cast<float>(spacing);
            int const x = static_cast<int>(std::round(original.x / s) * s);
            int const y = static_cast<int>(std::round(original.y / s) * s);
            return { x, y };
        } else {
            return original;
        }
    }

    bool BoundsWidget::OnMouseUp(EventMouseUp const& event) {
        if (event.GetButton() != MouseButton::Left) {
            return false;
        }
        if (m_holding) {
            m_holding = false;
            EndEdit();
        }
        return false;
    }

    bool BoundsWidget::OnMouseMove(EventMouseMove const& event) {
        if (m_holding) {
            if (auto const selection = m_editorLayer.GetSelection()) {
                auto const& canvasTopLeft = m_editorLayer.GetCanvasTopLeft();
                auto const windowMousePos = event.GetPosition();
                auto const canvasRelative = SnapToGrid(windowMousePos - canvasTopLeft);
                auto const delta = canvasRelative - m_grabPosition;
                m_grabPosition = canvasRelative;

                auto& bounds = selection->GetLayoutRect();
                bounds.offset.topLeft += static_cast<FloatVec2>(delta);
                bounds.offset.bottomRight += static_cast<FloatVec2>(delta);

                selection->RecalculateBounds();
            }
        }
        return false;
    }
}
