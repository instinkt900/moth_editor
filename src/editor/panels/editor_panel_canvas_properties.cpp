#include "common.h"
#include "editor_panel_canvas_properties.h"
#include "../editor_layer.h"

EditorPanelCanvasProperties::EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Canvas Properties", visible, true) {
}

void EditorPanelCanvasProperties::DrawContents() {
    auto& canvasSize = m_editorLayer.GetConfig().CanvasSize;
    float const separatorW = ImGui::CalcTextSize("x").x;
    float const labelW = ImGui::CalcTextSize("px").x;
    float const spacing = 4.0f;
    float const inputWidth = std::max(1.0f, (ImGui::GetContentRegionAvail().x - separatorW - labelW - (spacing * 4)) / 2.0f);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::PushID(&canvasSize.x);
    ImGui::InputInt("##w", &canvasSize.x, 0);
    ImGui::PopID();
    ImGui::SameLine(0, spacing);
    ImGui::Text("x");
    ImGui::SameLine(0, spacing);
    ImGui::SetNextItemWidth(inputWidth);
    ImGui::PushID(&canvasSize.y);
    ImGui::InputInt("##h", &canvasSize.y, 0);
    ImGui::PopID();
    ImGui::SameLine(0, spacing);
    ImGui::Text("px");
    canvasSize.x = std::max(1, canvasSize.x);
    canvasSize.y = std::max(1, canvasSize.y);
}
