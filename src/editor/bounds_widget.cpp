#include "common.h"
#include "bounds_widget.h"
#include "anchor_bounds_handle.h"
#include "offset_bounds_handle.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/group.h"
#include "moth_ui/node_image.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "editor_layer.h"

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

bool BoundsWidget::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
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
        auto const screenRect = static_cast<moth_ui::FloatRect>(selection->GetScreenRect());
        auto const scaleFactor = m_editorLayer.GetScaleFactor();
        auto const canvasPos = static_cast<moth_ui::IntVec2>(screenRect.topLeft / scaleFactor);
        auto const canvasSize = static_cast<moth_ui::IntVec2>((screenRect.bottomRight - screenRect.topLeft) / scaleFactor);

        auto layoutEntity = selection->GetLayoutEntity();
        if (layoutEntity && layoutEntity->GetType() == moth_ui::LayoutEntityType::Image) {
            auto imageNode = std::static_pointer_cast<moth_ui::NodeImage>(selection);
            auto imageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(layoutEntity);
            if (imageEntity->m_imageScaleType == moth_ui::ImageScaleType::NineSlice) {
                auto const slice1 = static_cast<moth_ui::IntVec2>(static_cast<moth_ui::FloatVec2>(imageNode->GetTargetSlices()[1]) / scaleFactor);
                auto const slice2 = static_cast<moth_ui::IntVec2>(static_cast<moth_ui::FloatVec2>(imageNode->GetTargetSlices()[2]) / scaleFactor);

                SDL_SetRenderDrawColor(&renderer, 0x77, 0x44, 0x00, 0xFF);
                SDL_RenderDrawLine(&renderer, slice1.x, canvasPos.y, slice1.x, canvasPos.y + canvasSize.y);
                SDL_RenderDrawLine(&renderer, slice2.x, canvasPos.y, slice2.x, canvasPos.y + canvasSize.y);
                SDL_RenderDrawLine(&renderer, canvasPos.x, slice1.y, canvasPos.x + canvasSize.x, slice1.y);
                SDL_RenderDrawLine(&renderer, canvasPos.x, slice2.y, canvasPos.x + canvasSize.x, slice2.y);
            }
        }

        SDL_Rect const sdlBoundsRect{ canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y };
        SDL_SetRenderDrawColor(&renderer, 0x00, 0x00, 0xFF, 0xFF);
        SDL_RenderDrawRect(&renderer, &sdlBoundsRect);

        for (auto& handle : m_handles) {
            handle->Draw(renderer);
        }
    }
}

bool BoundsWidget::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }

    std::shared_ptr<moth_ui::Node> selection;
    auto const& children = m_editorLayer.GetRoot()->GetChildren();
    for (auto it = std::rbegin(children); it != std::rend(children); ++it) {
        auto const& child = *it;
        if (child->IsVisible() && child->IsInBounds(event.GetPosition())) {
            selection = child;
            break;
        }
    }
    m_editorLayer.SetSelection(selection);
    for (auto&& handle : m_handles) {
        handle->SetTarget(selection.get());
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

moth_ui::IntVec2 BoundsWidget::SnapToGrid(moth_ui::IntVec2 const& original) {
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

bool BoundsWidget::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (event.GetButton() != moth_ui::MouseButton::Left) {
        return false;
    }
    if (m_holding) {
        m_holding = false;
        EndEdit();
    }
    return false;
}

bool BoundsWidget::OnMouseMove(moth_ui::EventMouseMove const& event) {
    if (m_holding) {
        if (auto const selection = m_editorLayer.GetSelection()) {
            auto const& canvasTopLeft = m_editorLayer.GetCanvasTopLeft();
            auto const windowMousePos = event.GetPosition();
            auto const canvasRelative = SnapToGrid(windowMousePos - canvasTopLeft);
            auto const delta = canvasRelative - m_grabPosition;
            m_grabPosition = canvasRelative;

            auto& bounds = selection->GetLayoutRect();
            bounds.offset.topLeft += static_cast<moth_ui::FloatVec2>(delta);
            bounds.offset.bottomRight += static_cast<moth_ui::FloatVec2>(delta);

            selection->RecalculateBounds();
        }
    }
    return false;
}
