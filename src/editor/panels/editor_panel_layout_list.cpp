#include "common.h"
#include "editor_panel_layout_list.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout.h"

EditorPanelLayoutList::EditorPanelLayoutList(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Layout List", visible, true)
    , m_contentList({}) {
    m_deleteConfirm.SetTitle("Delete?");
    m_deleteConfirm.SetMessage("Are you sure you wish to delete this file?");
    m_deleteConfirm.SetPositiveText("Delete");
    m_deleteConfirm.SetNegativeText("Cancel");

    m_contentList.SetDisplayNameAction([](std::filesystem::path const& path) {
        return path.filename().string();
    });

    m_contentList.SetDoubleClickAction([this](std::filesystem::path const& path) {
        m_editorLayer.LoadLayout(path.string().c_str());
    });

    m_contentList.SetChangeDirectoryAction([this](std::filesystem::path const& path) {
        auto& project = m_editorLayer.GetLayoutProject();
        project.m_layoutRoot = path.string();
    });

    m_contentList.SetDisplayFilter([](std::filesystem::path const& path) {
        if (std::filesystem::is_directory(path)) {
            return true;
        } else if (!path.has_extension() || path.extension() != moth_ui::Layout::Extension) {
            return false;
        }
        return true;
    });
}

void EditorPanelLayoutList::Refresh() {
    auto& projectInfo = m_editorLayer.GetLayoutProject();
    m_contentList.SetPath(projectInfo.m_layoutRoot);
    m_contentList.Refresh();
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
            auto const layoutFilename = fmt::format("{}{}", nameBuffer, moth_ui::Layout::Extension);
            auto const newLayoutPath = m_contentList.GetPath() / layoutFilename;
            if (std::filesystem::exists(newLayoutPath)) {
                fileExistsPopup = true;
            } else {
                auto const newLayout = std::make_shared<moth_ui::Layout>();
                auto const json = newLayout->Serialize({});
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
        auto const& currentSelection = m_contentList.GetCurrentSelection();
        if (std::filesystem::exists(currentSelection) && std::filesystem::is_regular_file(currentSelection)) {
            m_deleteConfirm.SetPositiveAction([this, currentSelection]() {
                std::filesystem::remove(currentSelection);
                Refresh();
            });
            m_deleteConfirm.Open();
        }
    }
    m_contentList.Draw();
}
