#include "common.h"
#include "editor_panel_canvas_properties.h"
#include "../editor_layer.h"

EditorPanelCanvasProperties::EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Canvas Properties", visible, true) {
}

void EditorPanelCanvasProperties::DrawContents() {
    auto& config = m_editorLayer.GetConfig();
    auto& canvasSize = config.CanvasSize;
    auto const& resolutions = config.Resolutions;

    // Find whether the current size matches a preset (used for both combo preview and focus).
    int matchIndex = -1;
    for (int i = 0; i < static_cast<int>(resolutions.size()); ++i) {
        if (resolutions[i].width == canvasSize.x && resolutions[i].height == canvasSize.y) {
            matchIndex = i;
            break;
        }
    }
    char const* const previewLabel = (matchIndex >= 0) ? resolutions[matchIndex].name.c_str() : "Custom";

    // Fixed-width number inputs; combo takes the remaining space.
    float const spacing = 4.0f;
    float const inputWidth = 100.0f;
    float const separatorW = ImGui::CalcTextSize("x").x;
    float const labelW = ImGui::CalcTextSize("px").x;
    float const fixedCost = (inputWidth * 2.0f) + separatorW + labelW + (spacing * 4.0f);
    float const comboW = resolutions.empty() ? 0.0f : std::max(80.0f, ImGui::GetContentRegionAvail().x - fixedCost);
    float const comboSpacing = resolutions.empty() ? 0.0f : spacing;

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

    if (!resolutions.empty()) {
        ImGui::SameLine(0, comboSpacing);
        ImGui::SetNextItemWidth(comboW);
        if (ImGui::BeginCombo("##preset", previewLabel)) {
            for (int i = 0; i < static_cast<int>(resolutions.size()); ++i) {
                auto const& res = resolutions[i];
                bool const selected = (i == matchIndex);
                if (ImGui::Selectable(res.name.c_str(), selected)) {
                    canvasSize.x = res.width;
                    canvasSize.y = res.height;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
}
