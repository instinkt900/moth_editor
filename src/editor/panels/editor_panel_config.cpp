#include "common.h"
#include "editor_panel_config.h"
#include "../editor_layer.h"

EditorPanelConfig::EditorPanelConfig(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Editor Config", visible, true)
    , m_config(editorLayer.GetConfig()) {
}

void EditorPanelConfig::DrawContents() {
    m_resetConfirm.Draw();

    ImGui::ColorEdit4("Canvas Background", m_config.CanvasBackgroundColor.data);
    ImGui::ColorEdit4("Canvas", m_config.CanvasColor.data);
    ImGui::ColorEdit4("Canvas Outline", m_config.CanvasOutlineColor.data);
    ImGui::ColorEdit4("Canvas Grid Minor", m_config.CanvasGridColorMinor.data);
    ImGui::ColorEdit4("Canvas Grid Major", m_config.CanvasGridColorMajor.data);
    ImGui::ColorEdit4("Selection", m_config.SelectionColor.data);
    ImGui::ColorEdit4("Selection Slice", m_config.SelectionSliceColor.data);
    ImGui::ColorEdit4("Preview Source Rect", m_config.PreviewSourceRectColor.data);
    ImGui::ColorEdit4("Preview Slice", m_config.PreviewImageSliceColor.data);

    if (ImGui::Button("Reset to default")) {
        m_resetConfirm.SetTitle("Reset Config?");
        m_resetConfirm.SetMessage("Reset all settings to default?");
        m_resetConfirm.SetPositiveAction([&]() {
            m_config = {};
        });
        m_resetConfirm.Open();
    }
}
