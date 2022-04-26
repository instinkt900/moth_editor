#include "common.h"
#include "editor_panel_project_properties.h"
#include "../editor_layer.h"
#include "editor_panel_asset_list.h"

#include "imgui-filebrowser/imfilebrowser.h"

#undef min
#undef max

namespace {
    enum class FileOpenMode {
        Unknown,
        LayoutRoot,
        ImageRoot,
    };
    FileOpenMode s_fileOpenMode = FileOpenMode::Unknown;
    ImGui::FileBrowser s_fileBrowser(ImGuiFileBrowserFlags_SelectDirectory);
}

EditorPanelProjectProperties::EditorPanelProjectProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Project Properties", visible, true) {
}

void EditorPanelProjectProperties::OnOpen() {
    m_tempProject = m_editorLayer.GetLayoutProject();
}

void EditorPanelProjectProperties::OnClose() {
    auto& layoutProject = m_editorLayer.GetLayoutProject();
    layoutProject = m_tempProject;

    m_editorLayer.GetEditorPanel<EditorPanelAssetList>()->Refresh();
}

void EditorPanelProjectProperties::DrawContents() {
    auto& layoutProject = m_editorLayer.GetLayoutProject();

    char tempBuffer[1024];
    memcpy(tempBuffer, m_tempProject.m_layoutRoot.c_str(), std::min(1024, static_cast<int>(m_tempProject.m_layoutRoot.size() + 1)));
    if (ImGui::InputText("Layout Root", tempBuffer, 1024)) {
        m_tempProject.m_layoutRoot = tempBuffer;
    }
    ImGui::SameLine();
    ImGui::PushID("layout_root_browse");
    if (ImGui::Button("...")) {
        s_fileOpenMode = FileOpenMode::LayoutRoot;
        s_fileBrowser.SetTitle("Select layout root..");
        s_fileBrowser.Open();
    }
    ImGui::PopID();

    memcpy(tempBuffer, m_tempProject.m_imageRoot.c_str(), std::min(1024, static_cast<int>(m_tempProject.m_imageRoot.size() + 1)));
    if (ImGui::InputText("Image Root", tempBuffer, 1024)) {
        m_tempProject.m_imageRoot = tempBuffer;
    }
    ImGui::SameLine();
    ImGui::PushID("image_root_browse");
    if (ImGui::Button("...")) {
        s_fileOpenMode = FileOpenMode::ImageRoot;
        s_fileBrowser.SetTitle("Select image root..");
        s_fileBrowser.Open();
    }
    ImGui::PopID();

    s_fileBrowser.Display();
    if (s_fileBrowser.HasSelected()) {
        if (s_fileOpenMode == FileOpenMode::LayoutRoot) {
            m_tempProject.m_layoutRoot = s_fileBrowser.GetSelected().string();
            s_fileBrowser.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::ImageRoot) {
            m_tempProject.m_imageRoot = s_fileBrowser.GetSelected().string();
            s_fileBrowser.ClearSelected();
        }
    }
}
