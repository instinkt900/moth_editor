#include "common.h"
#include "editor_panel_canvas_properties.h"
#include "../editor_layer.h"

namespace {
    static int constexpr s_maxZoom = 800;
    static int constexpr s_minZoom = 30;
}

EditorPanelCanvasProperties::EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Canvas Properties", visible, true) {
}

void EditorPanelCanvasProperties::DrawContents() {
    auto& canvasProperties = m_editorLayer.GetCanvasProperties();

    imgui_ext::InputIntVec2("Display Size", &canvasProperties.m_size);
    ImGui::InputInt("Display Zoom", &canvasProperties.m_zoom);
    canvasProperties.m_zoom = std::clamp(canvasProperties.m_zoom, s_minZoom, s_maxZoom);
    imgui_ext::InputFloatVec2("Display Offset", &canvasProperties.m_offset);
    ImGui::InputInt("Grid Spacing", &canvasProperties.m_gridSpacing);
    canvasProperties.m_gridSpacing = std::clamp(canvasProperties.m_gridSpacing, 0, canvasProperties.m_size.x / 2);
}
