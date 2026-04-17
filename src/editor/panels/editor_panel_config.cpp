#include "common.h"
#include "editor_panel_config.h"
#include "../editor_layer.h"

EditorPanelConfig::EditorPanelConfig(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Preferences", visible, true)
    , m_config(editorLayer.GetConfig()) {
}

bool EditorPanelConfig::BeginPanel() {
    ImGui::SetNextWindowSize({ 420.0f, 600.0f }, ImGuiCond_Appearing);
    return EditorPanel::BeginPanel();
}

void EditorPanelConfig::DrawContents() {
    m_resetConfirm.Draw();

    if (ImGui::CollapsingHeader("Canvas", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SetNextItemWidth(170.0f);
        ImGui::InputInt2("Size (px)", m_config.CanvasSize.data);
        m_config.CanvasSize.x = std::max(1, m_config.CanvasSize.x);
        m_config.CanvasSize.y = std::max(1, m_config.CanvasSize.y);

        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("Grid Spacing", &m_config.CanvasGridSpacing);
        m_config.CanvasGridSpacing = std::max(1, m_config.CanvasGridSpacing);

        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("Grid Major Factor", &m_config.CanvasGridMajorFactor);
        m_config.CanvasGridMajorFactor = std::max(1, m_config.CanvasGridMajorFactor);
    }

    if (ImGui::CollapsingHeader("Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
        static constexpr ImGuiColorEditFlags kColorFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf;
        if (ImGui::BeginTable("##colors", 2)) {
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Canvas Background",  m_config.CanvasBackgroundColor.data,  kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Canvas",             m_config.CanvasColor.data,            kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Canvas Outline",     m_config.CanvasOutlineColor.data,     kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Grid Minor",         m_config.CanvasGridColorMinor.data,   kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Grid Major",         m_config.CanvasGridColorMajor.data,   kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Selection",          m_config.SelectionColor.data,         kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Selection Slice",    m_config.SelectionSliceColor.data,    kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Preview Source Rect",m_config.PreviewSourceRectColor.data, kColorFlags);
            ImGui::TableNextColumn(); ImGui::ColorEdit4("Preview Slice",      m_config.PreviewImageSliceColor.data, kColorFlags);
            ImGui::EndTable();
        }
    }

    if (ImGui::CollapsingHeader("Snapping", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Snap to Grid", &m_config.SnapToGrid);
        ImGui::Checkbox("Snap to Angle", &m_config.SnapToAngle);
        if (m_config.SnapToAngle) {
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputFloat("Snap Angle", &m_config.SnapAngle, 0.0f, 0.0f, "%.1f");
        }
    }

    if (ImGui::CollapsingHeader("Auto-Save", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled", &m_config.AutoSaveEnabled);
        if (m_config.AutoSaveEnabled) {
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputInt("Interval (min)", &m_config.AutoSaveIntervalMinutes);
            m_config.AutoSaveIntervalMinutes = std::clamp(m_config.AutoSaveIntervalMinutes, 1, 1440);
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputInt("Max Versions", &m_config.AutoSaveMaxVersions);
            m_config.AutoSaveMaxVersions = std::clamp(m_config.AutoSaveMaxVersions, 1, 100);
        }
    }

    if (ImGui::CollapsingHeader("Sprite Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
        static constexpr ImGuiColorEditFlags kColorFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreviewHalf;
        ImGui::ColorEdit4("Normal",   m_config.SpriteEditorNormalColor.data,   kColorFlags);
        ImGui::ColorEdit4("Selected", m_config.SpriteEditorSelectedColor.data, kColorFlags);
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("Rect Thickness", &m_config.SpriteEditorRectThickness);
        m_config.SpriteEditorRectThickness = std::max(1, m_config.SpriteEditorRectThickness);
    }

    if (ImGui::CollapsingHeader("Resolution Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& resolutions = m_config.Resolutions;

        // Scrollable list with delete buttons.
        float const listHeight = std::min((static_cast<float>(resolutions.size()) * ImGui::GetFrameHeightWithSpacing()) + 4.0f, 160.0f);
        if (ImGui::BeginChild("##reslist", { 0.0f, listHeight }, ImGuiChildFlags_Border)) {
            int deleteIndex = -1;
            for (int i = 0; i < static_cast<int>(resolutions.size()); ++i) {
                ImGui::PushID(i);
                if (ImGui::SmallButton("X")) {
                    deleteIndex = i;
                }
                ImGui::SameLine();
                ImGui::TextUnformatted(resolutions[i].name.c_str());
                ImGui::PopID();
            }
            if (deleteIndex >= 0) {
                resolutions.erase(resolutions.begin() + deleteIndex);
            }
        }
        ImGui::EndChild();

        // Add form.
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputText("Name##resname", m_newResName, sizeof(m_newResName));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60.0f);
        ImGui::InputInt("W##resw", &m_newResWidth, 0);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(60.0f);
        ImGui::InputInt("H##resh", &m_newResHeight, 0);
        ImGui::SameLine();
        bool const canAdd = m_newResWidth > 0 && m_newResHeight > 0 && m_newResName[0] != '\0';
        if (!canAdd) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Add")) {
            resolutions.push_back({ m_newResName, m_newResWidth, m_newResHeight });
            m_newResName[0] = '\0';
            m_newResWidth = 0;
            m_newResHeight = 0;
        }
        if (!canAdd) {
            ImGui::EndDisabled();
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Reset to Default")) {
        m_resetConfirm.SetTitle("Reset Preferences?");
        m_resetConfirm.SetMessage("Reset all settings to their default values?");
        m_resetConfirm.SetPositiveAction([&]() {
            m_config = {};
        });
        m_resetConfirm.Open();
    }
}
