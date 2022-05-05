#include "common.h"
#include "editor_panel_canvas_properties.h"
#include "../editor_layer.h"
#include "../utils.h"
#include "editor_panel_canvas.h"

namespace {
    static int constexpr s_maxZoom = 800;
    static int constexpr s_minZoom = 30;
}

EditorPanelCanvasProperties::EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible, EditorPanelCanvas& canvasPanel)
    : EditorPanel(editorLayer, "Canvas Properties", visible, true)
    , m_canvasPanel(canvasPanel) {
}

void EditorPanelCanvasProperties::DrawContents() {
    imgui_ext::InputIntVec2("Canvas Size", &m_canvasPanel.m_canvasSize);
    ImGui::InputInt("Canvas Zoom", &m_canvasPanel.m_canvasZoom);
    m_canvasPanel.m_canvasZoom = std::clamp(m_canvasPanel.m_canvasZoom, s_minZoom, s_maxZoom);
    ImGui::InputInt("Grid Spacing", &m_canvasPanel.m_canvasGridSpacing);
}
