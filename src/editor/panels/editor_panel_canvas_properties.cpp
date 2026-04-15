#include "common.h"
#include "editor_panel_canvas_properties.h"
#include "../editor_layer.h"
#include "../imgui_ext.h"

EditorPanelCanvasProperties::EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Canvas Properties", visible, true) {
}

void EditorPanelCanvasProperties::DrawContents() {
    imgui_ext::InputIntVec2("Canvas Size", &m_editorLayer.GetConfig().CanvasSize);
}
