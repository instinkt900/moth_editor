#include "common.h"
#include "editor_panel_fonts.h"
#include "moth_ui/context.h"
#include "../editor_layer.h"

#include <nfd.h>

EditorPanelFonts::EditorPanelFonts(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Fonts", visible, true) {
}

void EditorPanelFonts::DrawContents() {
    static char NameBuffer[1024] = { 0 };
    auto fontNames = m_editorLayer.GetContext().GetFontFactory().GetFontNameList();

    auto const windowWidth = ImGui::GetContentRegionAvail().x;
    auto const buttonSize = ImVec2{ windowWidth / 2.0f - 5, 20 };
    if (ImGui::Button("Load List", buttonSize)) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog("json", currentPath.c_str(), &outPath);

        if (result == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            m_editorLayer.GetContext().GetFontFactory().LoadProject(filePath);
            fontNames = m_editorLayer.GetContext().GetFontFactory().GetFontNameList();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save List", buttonSize)) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_SaveDialog("json", currentPath.c_str(), &outPath);

        if (result == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            if (!filePath.has_extension()) {
                filePath.replace_extension(".json");
            }
            m_editorLayer.GetContext().GetFontFactory().SaveProject(filePath);
        }
    }
    if (ImGui::Button("Add Font", buttonSize)) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_SaveDialog("ttf", currentPath.c_str(), &outPath);

        if (result == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            m_pendingFontPath = filePath;
            NameBuffer[0] = 0;
            ImGui::OpenPopup("Name Font");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Font", buttonSize)) {
        if (m_selectedIndex >= 0) {

            m_editorLayer.GetContext().GetFontFactory().RemoveFont(fontNames[m_selectedIndex].c_str());
            fontNames = m_editorLayer.GetContext().GetFontFactory().GetFontNameList();
        }
    }
    ImGui::Columns(1);

    ImGui::PushID(this);
    if (ImGui::BeginListBox("##font_list", ImVec2(-FLT_MIN, 0))) {
        for (int i = 0; i < static_cast<int>(fontNames.size()); ++i) {
            auto const& entryInfo = fontNames[i];
            bool const selected = m_selectedIndex == i;
            if (ImGui::Selectable(entryInfo.c_str(), selected)) {
                m_selectedIndex = i;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopID();

    if (m_selectedIndex >= 0) {
        ImGui::Text("Absolute path:");
        std::string path = m_editorLayer.GetContext().GetFontFactory().GetFontPath(fontNames[m_selectedIndex].c_str()).string();
        ImGui::TextWrapped("%s", path.c_str());
    }

    if (ImGui::BeginPopupModal("Name Font", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Name this font. %s", m_pendingFontPath.filename().string().c_str());
        ImGui::InputText("Name", NameBuffer, 1024);
        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if (ImGui::Button("OK", button_size)) {
            m_editorLayer.GetContext().GetFontFactory().AddFont(NameBuffer, m_pendingFontPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
