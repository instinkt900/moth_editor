#include "common.h"
#include "editor_panel_fonts.h"
#include "moth_ui/context.h"

#include "imgui-filebrowser/imfilebrowser.h"

namespace {
    enum class FileDialogMode {
        UNKNOWN,
        LOAD_FONT_LIST,
        SAVE_FONT_LIST,
        ADD_FONT,
    };
    static ImGui::FileBrowser s_fileDialog(ImGuiFileBrowserFlags_EnterNewFilename);
    FileDialogMode s_fileMode = FileDialogMode::UNKNOWN;
}

EditorPanelFonts::EditorPanelFonts(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Fonts", visible, true) {
}

void EditorPanelFonts::DrawContents() {
    auto& fontFactory = moth_ui::Context::GetCurrentContext()->GetFontFactory();
    auto fontNames = fontFactory.GetFontNameList();

    if (ImGui::Button("Load Font List")) {
        s_fileMode = FileDialogMode::LOAD_FONT_LIST;
        s_fileDialog.SetTitle("Load font project..");
        s_fileDialog.SetTypeFilters({ ".json" });
        s_fileDialog.SetPwd();
        s_fileDialog.Open();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save Font List")) {
        s_fileMode = FileDialogMode::SAVE_FONT_LIST;
        s_fileDialog.SetTitle("Save font project..");
        s_fileDialog.SetTypeFilters({ ".json" });
        s_fileDialog.SetPwd();
        s_fileDialog.Open();
    }

    if (ImGui::Button("Add Font")) {
        s_fileMode = FileDialogMode::ADD_FONT;
        s_fileDialog.SetTitle("Select font..");
        s_fileDialog.SetTypeFilters({ ".ttf" });
        s_fileDialog.SetPwd();
        s_fileDialog.Open();
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Font")) {
        if (m_selectedIndex >= 0) {
            
            fontFactory.RemoveFont(fontNames[m_selectedIndex].c_str());
            fontNames = fontFactory.GetFontNameList();
        }
    }

    ImGui::PushID(this);
    if (ImGui::BeginListBox("##font_list", ImVec2(-FLT_MIN, -FLT_MIN))) {
        for (int i = 0; i < fontNames.size(); ++i) {
            auto const& entryInfo = fontNames[i];
            bool const selected = m_selectedIndex == i;
            if (ImGui::Selectable(entryInfo.c_str(), selected)) {
                m_selectedIndex = i;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopID();

    s_fileDialog.Display();
    if (s_fileDialog.HasSelected()) {
        switch (s_fileMode) {
        case FileDialogMode::LOAD_FONT_LIST: {
            auto filePath = s_fileDialog.GetSelected();
            fontFactory.LoadProject(filePath);
            fontNames = fontFactory.GetFontNameList();
            break;
        }
        case FileDialogMode::SAVE_FONT_LIST: {
            auto filePath = s_fileDialog.GetSelected();
            if (!filePath.has_extension()) {
                filePath.replace_extension(".json");
            }
            fontFactory.SaveProject(filePath);
            break;
        }
        case FileDialogMode::ADD_FONT: {
            m_pendingFontPath = s_fileDialog.GetSelected();
            ImGui::OpenPopup("Name Font");
            break;
        }
        }
        s_fileDialog.ClearSelected();
    }

    static char NameBuffer[1024] = { 0 };
    if (ImGui::BeginPopupModal("Name Font", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Name this font. %s", m_pendingFontPath.filename().string().c_str());
        ImGui::InputText("Name", NameBuffer, 1024);
        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if (ImGui::Button("OK", button_size)) {
            auto& fontFactory = moth_ui::Context::GetCurrentContext()->GetFontFactory();
            fontFactory.AddFont(NameBuffer, m_pendingFontPath);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}
