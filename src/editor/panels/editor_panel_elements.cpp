#include "common.h"
#include "editor_panel_elements.h"
#include "../editor_layer.h"

namespace {
    enum class FileOpenMode {
        Unknown,
        SubLayout,
        Image,
        NineSlice,
    };

    ImGui::FileBrowser s_fileBrowser;
    FileOpenMode s_fileOpenMode = FileOpenMode::Unknown;
}


EditorPanelElements::EditorPanelElements(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Elements", visible, true) {
}

void EditorPanelElements::DrawContents() {

    s_fileBrowser.Display();
    if (s_fileBrowser.HasSelected()) {
        if (s_fileOpenMode == FileOpenMode::SubLayout) {
            m_editorLayer.AddSubLayout(s_fileBrowser.GetSelected().string().c_str());
            s_fileBrowser.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::Image) {
            m_editorLayer.AddImage(s_fileBrowser.GetSelected().string().c_str());
            s_fileBrowser.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::NineSlice) {
            m_editorLayer.Add9Slice(s_fileBrowser.GetSelected().string().c_str());
            s_fileBrowser.ClearSelected();
        }
    }

    if (ImGui::Button("Rect")) {
        m_editorLayer.AddRect();
    } else if (ImGui::Button("Clip")) {
        m_editorLayer.AddClip();
    } else if (ImGui::Button("Image")) {
        s_fileBrowser.SetTitle("Open..");
        s_fileBrowser.SetTypeFilters({ ".jpg", ".jpeg", ".png", ".bmp" });
        s_fileBrowser.Open();
        s_fileOpenMode = FileOpenMode::Image;
    } else if (ImGui::Button("Text")) {
        m_editorLayer.AddText();
    } else if (ImGui::Button("SubLayout")) {
        s_fileBrowser.SetTitle("Open..");
        s_fileBrowser.SetTypeFilters({ ".json" });
        s_fileBrowser.Open();
        s_fileOpenMode = FileOpenMode::SubLayout;
    } else if (ImGui::Button("9Slice")) {
        s_fileBrowser.SetTitle("Open..");
        s_fileBrowser.SetTypeFilters({ ".jpg", ".jpeg", ".png", ".bmp" });
        s_fileBrowser.Open();
        s_fileOpenMode = FileOpenMode::NineSlice;
    }
}
