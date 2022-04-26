#include "common.h"
#include "editor_panel_canvas.h"
#include "app.h"
#include "editor/editor_layer.h"
#include "moth_ui/group.h"
#include "editor/bounds_widget.h"
#include "moth_ui/events/event_mouse.h"
#include "imgui_internal.h"
#include "../element_utils.h"
#include "moth_ui/layout/layout_entity_ref.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout.h"

extern App* g_App;

#ifndef IMGUI_DEFINE_MATH_OPERATORS
static ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}
static ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}
#endif

EditorPanelCanvas::EditorPanelCanvas(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Canvas", visible, false)
    , m_boundsWidget(std::make_unique<BoundsWidget>(editorLayer)) {
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
    UpdateDisplayTexture(*g_App->GetRenderer(), windowRegionSize);
    ImGui::Image(m_displayTexture.get(), ImVec2(static_cast<float>(windowRegionSize.x), static_cast<float>(windowRegionSize.y)));

    auto const windowPos = ImGui::GetWindowPos();
    auto const windowSize = ImGui::GetWindowSize();
    auto const windowContentMin = ImGui::GetWindowContentRegionMin();
    auto const windowContentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 const inputPadding{ 10, 10 };
    ImRect const windowContentRect{ windowPos + inputPadding, windowPos + windowSize - (inputPadding + inputPadding) };
    auto const mousePos = ImGui::GetMousePos();

    if (ImGui::IsWindowFocused() && windowContentRect.Contains(mousePos)) {
        auto& canvasProperties = m_editorLayer.GetCanvasProperties();
        float const scaleFactor = 100.0f / canvasProperties.m_zoom;

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
            if (!m_dragging) {
                m_dragging = true;
                m_initialCanvasOffset = canvasProperties.m_offset;
            }

            auto const dragDelta = moth_ui::FloatVec2{ ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).x, ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle).y };

            canvasProperties.m_offset = m_initialCanvasOffset + (moth_ui::FloatVec2{ dragDelta.x, dragDelta.y } / scaleFactor);
        } else {
            m_dragging = false;
        }

        auto const io = ImGui::GetIO();
        if (io.MouseWheel != 0) {
            canvasProperties.m_zoom += static_cast<int>(io.MouseWheel * 6 / scaleFactor);
        }

        moth_ui::FloatVec2 mousePosF{ mousePos.x - windowPos.x - windowContentMin.x, mousePos.y - windowPos.y - windowContentMin.y };
        if (!m_wasLeftDown && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_wasLeftDown = true;
            m_boundsWidget->OnEvent(moth_ui::EventMouseDown(moth_ui::MouseButton::Left, static_cast<moth_ui::IntVec2>(mousePosF * scaleFactor)));
        } else if (m_wasLeftDown && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_wasLeftDown = false;
            m_boundsWidget->OnEvent(moth_ui::EventMouseUp(moth_ui::MouseButton::Left, static_cast<moth_ui::IntVec2>(mousePosF * scaleFactor)));
        }
        if (mousePosF != m_lastMousePos) {
            m_boundsWidget->OnEvent(moth_ui::EventMouseMove(static_cast<moth_ui::IntVec2>(mousePosF * scaleFactor), (mousePosF - m_lastMousePos) * scaleFactor));
            m_lastMousePos = mousePosF;
        }
    }
}

void EditorPanelCanvas::UpdateDisplayTexture(SDL_Renderer& renderer, moth_ui::IntVec2 const displaySize) {
    if (!m_displayTexture || m_currentDisplaySize != displaySize) {
        m_currentDisplaySize = displaySize;
        m_displayTexture = CreateTextureRef(SDL_CreateTexture(g_App->GetRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, m_currentDisplaySize.x, m_currentDisplaySize.y));
    }

    auto const oldRenderTarget = SDL_GetRenderTarget(&renderer);
    SDL_SetRenderTarget(&renderer, m_displayTexture.get());
    SDL_SetRenderDrawColor(&renderer, 0xAA, 0xAA, 0xAA, 0xFF);
    SDL_RenderClear(&renderer);

    auto const& canvasProperties = m_editorLayer.GetCanvasProperties();

    float const scaleFactor = 100.0f / canvasProperties.m_zoom;

    auto const displaySizeF = static_cast<moth_ui::FloatVec2>(displaySize);
    auto const canvasSizeF = static_cast<moth_ui::FloatVec2>(canvasProperties.m_size);
    auto const preScaleSize = canvasSizeF / scaleFactor;
    auto const preScaleOffset = canvasProperties.m_offset + (displaySizeF - (canvasSizeF / scaleFactor)) / 2.0f;

    SDL_FRect canvasRect{ preScaleOffset.x,
                          preScaleOffset.y,
                          preScaleSize.x,
                          preScaleSize.y };

    // clear the canvas area
    {

        SDL_SetRenderDrawColor(&renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderFillRectF(&renderer, &canvasRect);
    }

    // grid lines
    {
        if (canvasProperties.m_gridSpacing > 0) {
            int const vertGridCount = (canvasProperties.m_size.x - 1) / canvasProperties.m_gridSpacing;
            int const horizGridCount = (canvasProperties.m_size.y - 1) / canvasProperties.m_gridSpacing;
            int index = 0;
            float gridX = canvasProperties.m_gridSpacing / scaleFactor;
            SDL_SetRenderDrawColor(&renderer, 0xDD, 0xDD, 0xDD, 0xFF);
            for (int i = 0; i < vertGridCount; ++i) {
                int const x = static_cast<int>(gridX);
                SDL_RenderDrawLineF(&renderer, canvasRect.x + x, canvasRect.y, canvasRect.x + x, canvasRect.y + canvasRect.h - 1);
                gridX += canvasProperties.m_gridSpacing / scaleFactor;
            }
            float gridY = canvasProperties.m_gridSpacing / scaleFactor;
            for (int i = 0; i < horizGridCount; ++i) {
                int const y = static_cast<int>(gridY);
                SDL_RenderDrawLineF(&renderer, canvasRect.x, canvasRect.y + y, canvasRect.x + canvasRect.w - 1, canvasRect.y + y);
                gridY += canvasProperties.m_gridSpacing / scaleFactor;
            }
        }
    }

    // setup scaling and draw the layout
    int oldRenderWidth, oldRenderHeight;
    SDL_RenderGetLogicalSize(&renderer, &oldRenderWidth, &oldRenderHeight);
    int const newRenderWidth = static_cast<int>(oldRenderWidth * scaleFactor);
    int const newRenderHeight = static_cast<int>(oldRenderHeight * scaleFactor);
    int const newRenderOffsetX = static_cast<int>(canvasProperties.m_offset.x * scaleFactor);
    int const newRenderOffsetY = static_cast<int>(canvasProperties.m_offset.y * scaleFactor);
    SDL_RenderSetLogicalSize(&renderer, newRenderWidth, newRenderHeight);
    {
        SDL_Rect guideRect{ newRenderOffsetX + (newRenderWidth - canvasProperties.m_size.x) / 2, newRenderOffsetY + (newRenderHeight - canvasProperties.m_size.y) / 2, canvasProperties.m_size.x, canvasProperties.m_size.y };
        m_editorLayer.GetCanvasProperties().m_topLeft = { guideRect.x, guideRect.y };
        if (auto const root = m_editorLayer.GetRoot()) {
            moth_ui::IntRect displayRect;
            displayRect.topLeft = { guideRect.x, guideRect.y };
            displayRect.bottomRight = { guideRect.x + guideRect.w, guideRect.y + guideRect.h };
            root->SetScreenRect(displayRect);
            root->Draw();
        }
    }
    SDL_RenderSetLogicalSize(&renderer, oldRenderWidth, oldRenderHeight); // reset logical sizing

    m_boundsWidget->Draw(renderer);

    SDL_SetRenderTarget(&renderer, oldRenderTarget);
}

void EditorPanelCanvas::EndPanel() {
    auto const windowPos = ImGui::GetWindowPos();
    auto const windowSize = ImGui::GetWindowSize();
    ImRect const windowContentRect{ windowPos + ImVec2{ 5, 5 }, windowPos + windowSize - ImVec2{ 5, 5 } };
    auto const windowID = ImGui::GetCurrentWindow()->ID;

    if (ImGui::BeginDragDropTargetCustom(windowContentRect, windowID)) {
        if (auto const payload = ImGui::AcceptDragDropPayload("layout_path", 0)) {
            std::string* layoutPath = static_cast<std::string*>(payload->Data);
            std::shared_ptr<moth_ui::Layout> newLayout;
            auto loadResult = moth_ui::Layout::Load(layoutPath->c_str(), &newLayout);
            if (loadResult == moth_ui::Layout::LoadResult::Success) {
                AddEntity<moth_ui::LayoutEntityRef>(m_editorLayer, *newLayout);
            }
        } else if (auto const payload = ImGui::AcceptDragDropPayload("image_path", 0)) {
            std::string* layoutPath = static_cast<std::string*>(payload->Data);
            AddEntity<moth_ui::LayoutEntityImage>(m_editorLayer, layoutPath->c_str());
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::End();
}
