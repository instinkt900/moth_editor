#include "common.h"
#include "editor_panel_canvas_properties.h"
#include "../editor_layer.h"
#include "../utils.h"
#include "editor_panel_canvas.h"
#include "../imgui_ext.h"
#include "imgui_internal.h"

namespace {
    static int constexpr s_maxZoom = 800;
    static int constexpr s_minZoom = 30;
}

EditorPanelCanvasProperties::EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible, EditorPanelCanvas& canvasPanel)
    : EditorPanel(editorLayer, "Canvas Properties", visible, true)
    , m_canvasPanel(canvasPanel) {
}

void EditorPanelCanvasProperties::DrawContents() {
    imgui_ext::InputIntVec2("Canvas Size", &m_editorLayer.GetConfig().CanvasSize);
    ImGui::InputInt("Canvas Zoom", &m_canvasPanel.m_canvasZoom);
    m_canvasPanel.m_canvasZoom = std::clamp(m_canvasPanel.m_canvasZoom, s_minZoom, s_maxZoom);
    ImGui::PushItemWidth(ImMax(1.0f, (ImGui::CalcItemWidth() - 6) / 2.0f));
    ImGui::InputInt("##Grid Spacing", &m_editorLayer.GetConfig().CanvasGridSpacing);
    ImGui::SameLine();
    ImGui::InputInt("Grid Spacing/Major Factor", &m_editorLayer.GetConfig().CanvasGridMajorFactor);
    ImGui::PopItemWidth();
}
