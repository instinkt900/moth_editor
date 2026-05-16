#include "common.h"
#include "editor_panel_reference_image.h"
#include "../editor_layer.h"

#include <nfd.h>

EditorPanelReferenceImage::EditorPanelReferenceImage(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

void EditorPanelReferenceImage::Draw() {
    if (!m_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(360, 0), ImVec2(FLT_MAX, FLT_MAX));
    if (!ImGui::Begin("Reference Image", &m_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    auto& config = m_editorLayer.GetConfig();

    ImGui::TextUnformatted("Image:");
    ImGui::SameLine();
    if (config.ReferenceImagePath.empty()) {
        ImGui::TextDisabled("(none)");
    } else {
        ImGui::TextWrapped("%s", config.ReferenceImagePath.c_str());
    }

    ImGui::Spacing();
    if (ImGui::Button("Load...")) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = nullptr;
        if (NFD_OpenDialog("png,jpg,jpeg,bmp,tga", currentPath.c_str(), &outPath) == NFD_OKAY) {
            config.ReferenceImagePath = outPath;
            NFD_Free(outPath);
        }
    }
    ImGui::SameLine();
    bool const hasImage = !config.ReferenceImagePath.empty();
    if (!hasImage) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Clear")) {
        config.ReferenceImagePath.clear();
    }
    if (!hasImage) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();

    ImGui::Checkbox("Visible", &config.ReferenceImageVisible);
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::SliderFloat("##opacity", &config.ReferenceImageOpacity, 0.0f, 1.0f, "Opacity: %.2f");

    ImGui::Spacing();
    float const closeW = 80.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - closeW);
    if (ImGui::Button("Close", ImVec2(closeW, 0))) {
        m_open = false;
    }

    ImGui::End();
}
