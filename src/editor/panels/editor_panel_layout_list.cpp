#include "common.h"
#include "editor_panel_layout_list.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout.h"

EditorPanelLayoutList::EditorPanelLayoutList(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Layout List", visible, true) {
    m_deleteConfirm.SetTitle("Delete?");
    m_deleteConfirm.SetMessage("Are you sure you wish to delete this file?");
    m_deleteConfirm.SetPositiveText("Delete");
    m_deleteConfirm.SetNegativeText("Cancel");
}

void EditorPanelLayoutList::Refresh() {
    m_selectedIndex = -1;
    m_layoutList.clear();
    auto& projectInfo = m_editorLayer.GetLayoutProject();
    for (auto& entry : std::filesystem::directory_iterator(projectInfo.m_layoutRoot)) {
        auto const& filePath = entry.path();
        if (filePath.has_extension()) {
            auto const fileExtension = filePath.extension().string();
            if (fileExtension == ".json") {
                LayoutInfo info{
                    filePath.string(),
                    filePath.stem().string()
                };
                m_layoutList.push_back(info);
            }
        }
    }
    m_currentPath = projectInfo.m_layoutRoot;
}

void EditorPanelLayoutList::DrawContents() {
    static char nameBuffer[1024] = { 0 };
    bool fileExistsPopup = false;

    m_deleteConfirm.Draw();

    if (ImGui::BeginPopupModal("File Exists", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("Layout already exists");
        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if (ImGui::Button("OK", button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopupModal("Layout Name", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##layout_name", nameBuffer, 1024);
        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if (ImGui::Button("OK", button_size)) {
            ImGui::CloseCurrentPopup();
            auto const layoutFilename = fmt::format("{}.json", nameBuffer);
            auto const newLayoutPath = m_currentPath / layoutFilename;
            if (std::filesystem::exists(newLayoutPath)) {
                fileExistsPopup = true;
            } else {
                auto const newLayout = std::make_shared<moth_ui::Layout>();
                auto const json = newLayout->Serialize();
                std::ofstream ofile(newLayoutPath.string());
                if (ofile.is_open()) {
                    ofile << json;
                }
                Refresh();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (fileExistsPopup) {
        ImGui::OpenPopup("File Exists");
    }

    if (ImGui::Button("New Layout")) {
        nameBuffer[0] = 0;
        ImGui::OpenPopup("Layout Name");
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete Layout")) {
        if (m_selectedIndex != -1) {
            m_deleteConfirm.SetPositiveAction([this]() {
                auto& layoutInfo = m_layoutList[m_selectedIndex];
                std::filesystem::remove(layoutInfo.m_path);
                Refresh();
            });
            m_deleteConfirm.Open();
        }
    }
    ImGui::BeginListBox("##layout_list", ImVec2(-FLT_MIN, -FLT_MIN));
    for (int i = 0; i < m_layoutList.size(); ++i) {
        auto& layoutInfo = m_layoutList[i];
        bool selected = m_selectedIndex == i;
        if (ImGui::Selectable(layoutInfo.m_name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            m_selectedIndex = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_editorLayer.LoadLayout(layoutInfo.m_path.c_str());
            }
        }
    }
    ImGui::EndListBox();
}
