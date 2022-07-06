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
    dispatch.Dispatch(this, &BoundsWidget::OnMouseDown);
    dispatch.Dispatch(this, &BoundsWidget::OnMouseUp);
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

void BoundsWidget::Draw() {
    if (m_node && m_node->IsVisible() && m_node->GetParent()) {
        auto const drawList = ImGui::GetWindowDrawList();
        auto const rect = m_canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, float>(m_node->GetScreenRect());

        m_anchorButtonTL.topLeft.x = rect.topLeft.x - (m_anchorButtonSize / 2);
        m_anchorButtonTL.topLeft.y = rect.topLeft.y - (m_anchorButtonSize * 2);
        m_anchorButtonTL.bottomRight.x = m_anchorButtonTL.topLeft.x + m_anchorButtonSize;
        m_anchorButtonTL.bottomRight.y = m_anchorButtonTL.topLeft.y + m_anchorButtonSize;

        m_anchorButtonFill = m_anchorButtonTL;
        m_anchorButtonFill += moth_ui::FloatVec2{ m_anchorButtonSize + m_anchorButtonSpacing, 0 };

        // 9 slice indicators
        auto const sliceColor = moth_ui::ToABGR(m_canvasPanel.GetEditorLayer().GetConfig().SelectionSliceColor);
        auto layoutEntity = m_node->GetLayoutEntity();
        if (layoutEntity && layoutEntity->GetType() == moth_ui::LayoutEntityType::Image) {
            auto imageNode = std::static_pointer_cast<moth_ui::NodeImage>(m_node);
            auto imageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(layoutEntity);
            if (imageEntity->m_imageScaleType == moth_ui::ImageScaleType::NineSlice) {
                auto const slice1 = m_canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, float>(imageNode->GetTargetSlices()[1]);
                auto const slice2 = m_canvasPanel.ConvertSpace<EditorPanelCanvas::CoordSpace::WorldSpace, EditorPanelCanvas::CoordSpace::AppSpace, float>(imageNode->GetTargetSlices()[2]);

                drawList->AddLine(ImVec2{ slice1.x, rect.topLeft.y }, ImVec2{ slice1.x, rect.bottomRight.y }, sliceColor);
                drawList->AddLine(ImVec2{ slice2.x, rect.topLeft.y }, ImVec2{ slice2.x, rect.bottomRight.y }, sliceColor);
                drawList->AddLine(ImVec2{ rect.topLeft.x, slice1.y }, ImVec2{ rect.bottomRight.x, slice1.y }, sliceColor);
                drawList->AddLine(ImVec2{ rect.topLeft.x, slice2.y }, ImVec2{ rect.bottomRight.x, slice2.y }, sliceColor);
            }
        }

        // overall bounds
        auto const boundsColor = m_canvasPanel.GetEditorLayer().GetConfig().SelectionColor;
        drawList->AddRect(ImVec2{ rect.topLeft.x, rect.topLeft.y }, ImVec2{ rect.bottomRight.x, rect.bottomRight.y },  moth_ui::ToABGR(boundsColor));

        // anchor preset buttons
        auto const buttonColor = moth_ui::Color{ boundsColor.r, boundsColor.g, boundsColor.b, 0.5f };
        drawList->AddRectFilled(ImVec2{ m_anchorButtonTL.topLeft.x, m_anchorButtonTL.topLeft.y }, ImVec2{ m_anchorButtonTL.bottomRight.x, m_anchorButtonTL.bottomRight.y }, moth_ui::ToABGR(buttonColor));
        drawList->AddRectFilled(ImVec2{ m_anchorButtonTL.topLeft.x, m_anchorButtonTL.topLeft.y }, ImVec2{ m_anchorButtonTL.topLeft.x+4, m_anchorButtonTL.topLeft.y+4 }, 0xFF000000);
        drawList->AddRectFilled(ImVec2{ m_anchorButtonFill.topLeft.x, m_anchorButtonFill.topLeft.y }, ImVec2{ m_anchorButtonFill.bottomRight.x, m_anchorButtonFill.bottomRight.y }, moth_ui::ToABGR(buttonColor));
        drawList->AddRect(ImVec2{ m_anchorButtonFill.topLeft.x+2, m_anchorButtonFill.topLeft.y+2 }, ImVec2{ m_anchorButtonFill.bottomRight.x-2, m_anchorButtonFill.bottomRight.y-2 }, 0xFF000000, 0, 0, 4);

        for (auto& handle : m_handles) {
            handle->Draw();
        }
    }
}

void BoundsWidget::SetSelection(std::shared_ptr<moth_ui::Node> node) {
    m_node = node;
    for (auto&& handle : m_handles) {
        handle->SetTarget(m_node.get());
    }
}

bool BoundsWidget::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (IsInRect(event.GetPosition(), m_anchorButtonTL)) {
        m_anchorTLPressed = true;
        return true;
    } else if (IsInRect(event.GetPosition(), m_anchorButtonFill)) {
        m_anchorFillPressed = true;
        return true;
    }
    return false;
}

bool BoundsWidget::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (IsInRect(event.GetPosition(), m_anchorButtonTL) && m_anchorTLPressed) {
        m_canvasPanel.GetEditorLayer().BeginEditBounds();
        auto& rect = m_node->GetLayoutRect();
        auto const& screenRect = m_node->GetScreenRect();
        auto const& parentRect = m_node->GetParent()->GetScreenRect();
        auto const newTLOffsetX = screenRect.topLeft.x - (parentRect.w() * 0) - parentRect.topLeft.x;
        auto const newTLOffsetY = screenRect.topLeft.y - (parentRect.h() * 0) - parentRect.topLeft.y;
        auto const newBROffsetX = screenRect.bottomRight.x - (parentRect.w() * 0) - parentRect.topLeft.x;
        auto const newBROffsetY = screenRect.bottomRight.y - (parentRect.h() * 0) - parentRect.topLeft.y;
        rect.anchor.topLeft = { 0, 0 };
        rect.anchor.bottomRight = { 0, 0 };
        rect.offset.topLeft = { newTLOffsetX, newTLOffsetY };
        rect.offset.bottomRight = { newBROffsetX, newBROffsetY };
        m_canvasPanel.GetEditorLayer().EndEditBounds();
        m_node->RecalculateBounds();
        m_anchorTLPressed = false;
        return true;
    } else if (IsInRect(event.GetPosition(), m_anchorButtonFill) && m_anchorFillPressed) {
        m_canvasPanel.GetEditorLayer().BeginEditBounds();
        auto& rect = m_node->GetLayoutRect();
        auto const& screenRect = m_node->GetScreenRect();
        auto const& parentRect = m_node->GetParent()->GetScreenRect();
        auto const newTLOffsetX = screenRect.topLeft.x - (parentRect.w() * 0) - parentRect.topLeft.x;
        auto const newTLOffsetY = screenRect.topLeft.y - (parentRect.h() * 0) - parentRect.topLeft.y;
        auto const newBROffsetX = screenRect.bottomRight.x - (parentRect.w() * 1) - parentRect.topLeft.x;
        auto const newBROffsetY = screenRect.bottomRight.y - (parentRect.h() * 1) - parentRect.topLeft.y;
        rect.anchor.topLeft = { 0, 0 };
        rect.anchor.bottomRight = { 1, 1 };
        rect.offset.topLeft = { newTLOffsetX, newTLOffsetY };
        rect.offset.bottomRight = { newBROffsetX, newBROffsetY };
        m_canvasPanel.GetEditorLayer().EndEditBounds();
        m_node->RecalculateBounds();
        m_anchorFillPressed = false;
        return true;
    }
    return false;
}
