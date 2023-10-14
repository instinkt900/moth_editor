#include "common.h"
#include "editor_panel_canvas.h"
#include "editor/editor_layer.h"
#include "moth_ui/group.h"
#include "editor/bounds_widget.h"
#include "moth_ui/events/event_mouse.h"
#include "imgui_internal.h"
#include "../element_utils.h"
#include "moth_ui/layout/layout_entity_ref.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout.h"
#include "../actions/composite_action.h"
#include "imgui_internal.h"
#include "editor_application.h"
#include "moth_ui/itarget.h"

EditorPanelCanvas::EditorPanelCanvas(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Canvas", visible, false)
    , m_boundsWidget(std::make_unique<BoundsWidget>(*this)) {
}

void EditorPanelCanvas::OnShutdown() {
}

bool EditorPanelCanvas::BeginPanel() {
    ImGui::SetNextWindowDockID(m_editorLayer.GetDockID());
    ImGuiWindowFlags host_window_flags = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    bool result = ImGui::Begin(m_title.c_str(), nullptr, host_window_flags);
    ImGui::PopStyleVar();
    return result;
}

void EditorPanelCanvas::DrawContents() {
    moth_ui::IntVec2 const windowRegionSize{ static_cast<int>(ImGui::GetContentRegionAvail().x), static_cast<int>(ImGui::GetContentRegionAvail().y) };

    UpdateDisplayTexture(windowRegionSize);
    imgui_ext::Image(m_displayTexture->GetImage(), windowRegionSize.x, windowRegionSize.y);

    auto const drawList = ImGui::GetWindowDrawList();

    // draw selected item rects
    {
        auto const& selection = m_editorLayer.GetSelection();
        for (auto&& node : selection) {
            if (node->IsVisible() && node->GetParent()) {
                auto const rect = ConvertSpace<CoordSpace::WorldSpace, CoordSpace::AppSpace, float>(node->GetScreenRect());
                drawList->AddRect(ImVec2{ rect.topLeft.x, rect.topLeft.y }, ImVec2{ rect.bottomRight.x, rect.bottomRight.y }, 0xFFFF00FF);
            }
        }
    }

    // draw the selection rect if we have one
    {
        if (m_dragSelecting) {
            auto const minX = static_cast<float>(std::min(m_dragSelectStart.x, m_dragSelectEnd.x));
            auto const maxX = static_cast<float>(std::max(m_dragSelectStart.x, m_dragSelectEnd.x));
            auto const minY = static_cast<float>(std::min(m_dragSelectStart.y, m_dragSelectEnd.y));
            auto const maxY = static_cast<float>(std::max(m_dragSelectStart.y, m_dragSelectEnd.y));
            if ((maxX - minX) > 3 || (maxY - minY) > 3) {
                drawList->AddRect(ImVec2{ minX, minY }, ImVec2{ maxX, maxY }, 0xFFFFFFFF);
            }
        }
    }

    auto const selection = m_editorLayer.GetSelection();
    auto const firstIt = std::begin(selection);
    if (selection.size() == 1 && !m_editorLayer.IsLocked(*firstIt)) {
        m_boundsWidget->SetSelection(*firstIt);
    } else {
        m_boundsWidget->SetSelection(nullptr);
    }
    m_boundsWidget->Draw();

    UpdateInput();
}

void EditorPanelCanvas::UpdateDisplayTexture(moth_ui::IntVec2 const& windowSize) {
    auto& graphics = g_App->GetGraphics();

    if (!m_displayTexture || m_canvasWindowSize != windowSize) {
        m_displayTexture = graphics.CreateTarget(windowSize.x, windowSize.y);
    }

    m_canvasWindowPos = moth_ui::IntVec2{ static_cast<int>(ImGui::GetWindowPos().x), static_cast<int>(ImGui::GetWindowPos().y) };
    m_canvasWindowSize = windowSize;

    // auto const oldRenderTarget = graphics.GetTarget();
    graphics.SetTarget(m_displayTexture.get());

    auto const& canvasSize = m_editorLayer.GetConfig().CanvasSize;

    // clear the window
    {
        graphics.SetColor(m_editorLayer.GetConfig().CanvasBackgroundColor);
        graphics.Clear();
    }

    // clear the canvas area
    {
        graphics.SetColor(m_editorLayer.GetConfig().CanvasColor);
        moth_ui::IntRect canvasRect;
        canvasRect.topLeft = { 0, 0 };
        canvasRect.bottomRight = canvasSize;
        auto const rect = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace, float>(canvasRect);
        graphics.DrawFillRectF(rect);
    }

    // grid lines
    {
        graphics.SetBlendMode(moth_ui::BlendMode::Alpha);
        auto const& gridSpacing = m_editorLayer.GetConfig().CanvasGridSpacing;
        auto const& gridMajorFactor = m_editorLayer.GetConfig().CanvasGridMajorFactor;
        if (gridSpacing > 0) {
            int i = 1;
            for (int x = gridSpacing; x < canvasSize.x; x += gridSpacing, ++i) {
                moth_ui::IntVec2 const p0{ x, 0 };
                moth_ui::IntVec2 const p1{ x, canvasSize.y };
                auto const p0Scaled = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace, float>(p0);
                auto const p1Scaled = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace, float>(p1);
                if ((i % gridMajorFactor) == 0) {
                    graphics.SetColor(m_editorLayer.GetConfig().CanvasGridColorMajor);
                } else {
                    graphics.SetColor(m_editorLayer.GetConfig().CanvasGridColorMinor);
                }
                graphics.DrawLineF(p0Scaled, p1Scaled);
            }
            i = 1;
            for (int y = gridSpacing; y < canvasSize.y; y += gridSpacing, ++i) {
                moth_ui::IntVec2 const p0{ 0, y };
                moth_ui::IntVec2 const p1{ canvasSize.x, y };
                auto const p0Scaled = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace, float>(p0);
                auto const p1Scaled = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace, float>(p1);
                if ((i % gridMajorFactor) == 0) {
                    graphics.SetColor(m_editorLayer.GetConfig().CanvasGridColorMajor);
                } else {
                    graphics.SetColor(m_editorLayer.GetConfig().CanvasGridColorMinor);
                }
                graphics.DrawLineF(p0Scaled, p1Scaled);
            }
        }
    }

    // outline the canvas
    {
        graphics.SetBlendMode(moth_ui::BlendMode::Alpha);
        graphics.SetColor(m_editorLayer.GetConfig().CanvasOutlineColor);
        moth_ui::IntRect canvasRect;
        canvasRect.topLeft = { 0, 0 };
        canvasRect.bottomRight = canvasSize;
        auto const rect = ConvertSpace<CoordSpace::CanvasSpace, CoordSpace::WorldSpace, float>(canvasRect);
        graphics.DrawRectF(rect);
    }

    // setup scaling and draw the layout
    {
        auto const scaleFactor = m_canvasZoom / 100.0f;
        auto const newRenderWidth = static_cast<int>(windowSize.x / scaleFactor);
        auto const newRenderHeight = static_cast<int>(windowSize.y / scaleFactor);
        auto const newRenderOffsetX = static_cast<int>(m_canvasOffset.x / scaleFactor);
        auto const newRenderOffsetY = static_cast<int>(m_canvasOffset.y / scaleFactor);

        graphics.SetBlendMode(moth_ui::BlendMode::Replace);
        graphics.SetLogicalSize(moth_ui::IntVec2{ newRenderWidth, newRenderHeight });
        {

            if (auto const root = m_editorLayer.GetRoot()) {
                moth_ui::IntRect const guideRect = moth_ui::MakeRect(
                    newRenderOffsetX + (newRenderWidth - canvasSize.x) / 2,
                    newRenderOffsetY + (newRenderHeight - canvasSize.y) / 2,
                    canvasSize.x,
                    canvasSize.y);
                root->SetScreenRect(guideRect);
                root->Draw();
            }
        }
        graphics.SetLogicalSize(moth_ui::IntVec2{ m_canvasWindowSize.x, m_canvasWindowSize.y }); // reset logical sizing
    }

    graphics.SetTarget(nullptr);
}

void EditorPanelCanvas::EndPanel() {
    auto const windowPos = ImGui::GetWindowPos();
    auto const windowSize = ImGui::GetWindowSize();
    ImRect const windowContentRect{ windowPos + ImVec2{ 5, 5 }, windowPos + windowSize - ImVec2{ 5, 5 } };
    auto const windowID = ImGui::GetCurrentWindow()->ID;

    if (ImGui::BeginDragDropTargetCustom(windowContentRect, windowID)) {
        auto const mousePos = moth_ui::IntVec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };
        auto const canvasPosition = ConvertSpace<CoordSpace::AppSpace, CoordSpace::CanvasSpace, int>(mousePos);

        if (auto const payload = ImGui::AcceptDragDropPayload("layout_path", 0)) {
            std::string* layoutPath = static_cast<std::string*>(payload->Data);
            std::shared_ptr<moth_ui::Layout> newLayout;
            auto loadResult = moth_ui::Layout::Load(layoutPath->c_str(), &newLayout);
            if (loadResult == moth_ui::Layout::LoadResult::Success) {
                moth_ui::LayoutRect bounds;
                bounds.anchor.topLeft = { 0, 0 };
                bounds.anchor.bottomRight = { 0, 0 };
                bounds.offset.topLeft = { canvasPosition.x, canvasPosition.y };
                bounds.offset.bottomRight = { canvasPosition.x + 100, canvasPosition.y + 100 };
                AddEntityWithBounds<moth_ui::LayoutEntityRef>(m_editorLayer, bounds, *newLayout);
            }
        } else if (auto const payload = ImGui::AcceptDragDropPayload("image_path", 0)) {
            std::string* layoutPath = static_cast<std::string*>(payload->Data);
            moth_ui::LayoutRect bounds;
            bounds.anchor.topLeft = { 0, 0 };
            bounds.anchor.bottomRight = { 0, 0 };
            bounds.offset.topLeft = { canvasPosition.x, canvasPosition.y };
            bounds.offset.bottomRight = { canvasPosition.x + 100, canvasPosition.y + 100 };
            AddEntityWithBounds<moth_ui::LayoutEntityImage>(m_editorLayer, bounds, layoutPath->c_str());
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}

moth_ui::IntVec2 EditorPanelCanvas::SnapToGrid(moth_ui::IntVec2 const& original) {
    auto const& gridSpacing = m_editorLayer.GetConfig().CanvasGridSpacing;
    if (gridSpacing > 0) {
        float const s = static_cast<float>(gridSpacing);
        int const x = static_cast<int>(std::round(original.x / s) * s);
        int const y = static_cast<int>(std::round(original.y / s) * s);
        return { x, y };
    } else {
        return original;
    }
}

void EditorPanelCanvas::ResetView() {
    m_canvasOffset = { 0, 0 };
    m_canvasZoom = 100;
}

void EditorPanelCanvas::BeginSelectionGrab(moth_ui::IntVec2 const& worldPosition) {
    m_holdingSelection = true;
    m_grabPosition = SnapToGrid(worldPosition);
    m_editorLayer.BeginEditBounds();
}

void EditorPanelCanvas::EndSelectionGrab() {
    m_editorLayer.EndEditBounds();
    m_holdingSelection = false;
}

void EditorPanelCanvas::OnMouseClicked(moth_ui::IntVec2 const& appPosition) {
    bool handled = m_boundsWidget->OnEvent(moth_ui::EventMouseDown(moth_ui::MouseButton::Left, appPosition));

    if (!handled && !m_holdingSelection) {
        auto const worldPosition = ConvertSpace<CoordSpace::AppSpace, CoordSpace::WorldSpace, int>(appPosition);

        // grab the node we clicked on, if any
        auto clickedNode = GetAtPoint(worldPosition);

        bool clickedSelection = false;

        if (!m_editorLayer.IsSelected(clickedNode)) {
            if (!ImGui::GetIO().KeyCtrl) {
                m_editorLayer.ClearSelection();
            }
            if (clickedNode) {
                m_editorLayer.AddSelection(clickedNode);
                clickedSelection = true;
            }
        } else {
            // next see if we clicked on an existing selection
            auto const selection = m_editorLayer.GetSelection();
            for (auto&& node : selection) {
                if (node->IsInBounds(worldPosition)) {
                    // clicked on an existing selection
                    clickedSelection = true;
                    break;
                }
            }
        }

        if (clickedSelection) {
            // clicked something that is selected now. begin possible move
            BeginSelectionGrab(worldPosition);
            handled = true;
        }
    }

    if (!handled && !m_dragSelecting) {
        m_dragSelecting = true;
        m_dragSelectStart = appPosition;
        m_dragSelectEnd = appPosition;
    }
}

void EditorPanelCanvas::OnMouseReleased(moth_ui::IntVec2 const& appPosition) {
    bool handled = m_boundsWidget->OnEvent(moth_ui::EventMouseUp(moth_ui::MouseButton::Left, appPosition));

    if (m_holdingSelection) {
        EndSelectionGrab();
    }

    if (!handled && m_dragSelecting) {
        m_dragSelectEnd = appPosition;

        auto const minX = std::min(m_dragSelectStart.x, m_dragSelectEnd.x);
        auto const maxX = std::max(m_dragSelectStart.x, m_dragSelectEnd.x);
        auto const minY = std::min(m_dragSelectStart.y, m_dragSelectEnd.y);
        auto const maxY = std::max(m_dragSelectStart.y, m_dragSelectEnd.y);

        moth_ui::IntRect selectionRect;
        selectionRect.topLeft = ConvertSpace<CoordSpace::AppSpace, CoordSpace::WorldSpace, int>(moth_ui::IntVec2{ minX, minY });
        selectionRect.bottomRight = ConvertSpace<CoordSpace::AppSpace, CoordSpace::WorldSpace, int>(moth_ui::IntVec2{ maxX, maxY });

        if (!ImGui::GetIO().KeyCtrl) {
            m_editorLayer.ClearSelection();
        }
        SelectInRect(selectionRect);
    }

    m_dragSelecting = false;
}

void EditorPanelCanvas::OnMouseMoved(moth_ui::IntVec2 const& appPosition) {
    m_boundsWidget->OnEvent(moth_ui::EventMouseMove(appPosition, static_cast<moth_ui::FloatVec2>(appPosition - m_lastMousePos)));

    if (m_holdingSelection) {
        auto const worldPosition = ConvertSpace<CoordSpace::AppSpace, CoordSpace::WorldSpace, int>(appPosition);
        auto const newPosition = SnapToGrid(worldPosition);
        auto const delta = newPosition - m_grabPosition;
        m_grabPosition = newPosition;

        auto const selection = m_editorLayer.GetSelection();
        for (auto&& node : selection) {
            auto& bounds = node->GetLayoutRect();
            bounds.offset.topLeft += static_cast<moth_ui::FloatVec2>(delta);
            bounds.offset.bottomRight += static_cast<moth_ui::FloatVec2>(delta);
            node->RecalculateBounds();
        }
        m_dragSelecting = false;
    }

    if (m_dragSelecting) {
        m_dragSelectEnd = appPosition;
    }
}

std::shared_ptr<moth_ui::Node> EditorPanelCanvas::GetAtPoint(moth_ui::IntVec2 const& selectionPoint) {
    auto const& children = m_editorLayer.GetRoot()->GetChildren();
    for (auto it = std::rbegin(children); it != std::rend(children); ++it) {
        auto const& child = *it;
        if (child->IsVisible() && !m_editorLayer.IsLocked(child)) {
            auto const& screenRect = child->GetScreenRect();
            if (moth_ui::IsInRect(selectionPoint, screenRect)) {
                return child;
            }
        }
    }
    return nullptr;
}

void EditorPanelCanvas::SelectInRect(moth_ui::IntRect const& selectionRect) {
    auto const& children = m_editorLayer.GetRoot()->GetChildren();
    for (auto it = std::rbegin(children); it != std::rend(children); ++it) {
        auto const& child = *it;
        if (child->IsVisible() && !m_editorLayer.IsLocked(child)) {
            auto const& screenRect = child->GetScreenRect();
            if (moth_ui::Intersects(selectionRect, screenRect)) {
                if (m_editorLayer.IsSelected(child)) {
                    m_editorLayer.RemoveSelection(child);
                } else {
                    m_editorLayer.AddSelection(child);
                }
            }
        }
    }
}

void EditorPanelCanvas::UpdateInput() {
    auto const mousePos = moth_ui::IntVec2{ ImGui::GetMousePos().x, ImGui::GetMousePos().y };

    if (ImGui::IsWindowHovered()) {
        float const scaleFactor = m_canvasZoom / 100.0f;

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            if (!m_draggingCanvas) {
                m_draggingCanvas = true;
                m_initialCanvasOffset = m_canvasOffset;
            }

            auto const dragDelta = moth_ui::FloatVec2{ ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).x, ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).y };

            m_canvasOffset = m_initialCanvasOffset + (moth_ui::FloatVec2{ dragDelta.x, dragDelta.y });
        } else {
            m_draggingCanvas = false;
        }

        auto const io = ImGui::GetIO();
        if (io.MouseWheel != 0) {
            m_canvasZoom += static_cast<int>(io.MouseWheel * 6 * scaleFactor);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            OnMouseClicked(moth_ui::IntVec2{ mousePos.x, mousePos.y });
        }
        if (m_lastMousePos != mousePos) {
            OnMouseMoved(moth_ui::IntVec2{ mousePos.x, mousePos.y });
            m_lastMousePos = mousePos;
        }
    }

    // always want to accept released so we dont end up stuck down
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        OnMouseReleased(moth_ui::IntVec2{ mousePos.x, mousePos.y });
    }
}
