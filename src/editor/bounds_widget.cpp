#include "common.h"
#include "bounds_widget.h"
#include "anchor_bounds_handle.h"
#include "offset_bounds_handle.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/group.h"
#include "moth_ui/node_image.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "editor_layer.h"
#include "panels/editor_panel_canvas.h"

BoundsWidget::BoundsWidget(EditorPanelCanvas& canvasPanel)
    : m_canvasPanel(canvasPanel) {
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
    return dispatch.GetHandled();
}

void BoundsWidget::BeginEdit() {
    m_canvasPanel.GetEditorLayer().BeginEditBounds();
}

void BoundsWidget::EndEdit() {
    m_canvasPanel.GetEditorLayer().EndEditBounds();
}

void BoundsWidget::Draw(SDL_Renderer& renderer) {
    if (m_node && m_node->IsVisible() && m_node->GetParent()) {
        auto const drawList = ImGui::GetWindowDrawList();
        auto const rect = m_canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, float>(m_node->GetScreenRect());

        // 9 slice indicators
        auto layoutEntity = m_node->GetLayoutEntity();
        if (layoutEntity && layoutEntity->GetType() == moth_ui::LayoutEntityType::Image) {
            auto imageNode = std::static_pointer_cast<moth_ui::NodeImage>(m_node);
            auto imageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(layoutEntity);
            if (imageEntity->m_imageScaleType == moth_ui::ImageScaleType::NineSlice) {
                auto const slice1 = m_canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, float>(imageNode->GetTargetSlices()[1]);
                auto const slice2 = m_canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, float>(imageNode->GetTargetSlices()[2]);

                drawList->AddLine(ImVec2{ slice1.x, rect.topLeft.y }, ImVec2{ slice1.x, rect.bottomRight.y }, 0xFF004477);
                drawList->AddLine(ImVec2{ slice2.x, rect.topLeft.y }, ImVec2{ slice2.x, rect.bottomRight.y }, 0xFF004477);
                drawList->AddLine(ImVec2{ rect.topLeft.x, slice1.y }, ImVec2{ rect.bottomRight.x, slice1.y }, 0xFF004477);
                drawList->AddLine(ImVec2{ rect.topLeft.x, slice2.y }, ImVec2{ rect.bottomRight.x, slice2.y }, 0xFF004477);
            }
        }

        // overall bounds
        drawList->AddRect(ImVec2{ rect.topLeft.x, rect.topLeft.y }, ImVec2{ rect.bottomRight.x, rect.bottomRight.y }, 0xFFFF0000);

        for (auto& handle : m_handles) {
            handle->Draw(renderer);
        }
    }
}

void BoundsWidget::SetSelection(std::shared_ptr<moth_ui::Node> node) {
    m_node = node;
    for (auto&& handle : m_handles) {
        handle->SetTarget(m_node.get());
    }
}
