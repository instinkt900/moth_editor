#include "common.h"
#include "editor_panel_asset_list.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout.h"

static std::string DragDropString;

namespace {
    static std::vector<std::string> const s_supportedExtensions{
        ".jpg",
        ".jpeg",
        ".png"
    };

    bool IsSupportedExtension(std::string const& ext) {
        return ext == moth_ui::Layout::Extension || std::end(s_supportedExtensions) != ranges::find(s_supportedExtensions, ext);
    }
}

EditorPanelAssetList::EditorPanelAssetList(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Asset List", visible, true)
    , m_contentList({}) {
    m_deleteConfirm.SetTitle("Delete?");
    m_deleteConfirm.SetMessage("Are you sure you wish to delete this file?");
    m_deleteConfirm.SetPositiveText("Delete");
    m_deleteConfirm.SetNegativeText("Cancel");

    m_contentList.SetDisplayNameAction([](std::filesystem::path const& path) {
        return path.filename().string();
    });

    m_contentList.SetDoubleClickAction([this](std::filesystem::path const& path) {
        if (path.extension().string() == moth_ui::Layout::Extension) {
            m_editorLayer.LoadLayout(path.string().c_str());
        }
    });

    m_contentList.SetChangeDirectoryAction([this](std::filesystem::path const& path) {
        auto& project = m_editorLayer.GetLayoutProject();
        project.m_layoutRoot = path.string();
    });

    m_contentList.SetPerEntryAction([](std::filesystem::path const& path) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            DragDropString = path.string();
            if (path.extension().string() == moth_ui::Layout::Extension) {
                ImGui::SetDragDropPayload("layout_path", &DragDropString, sizeof(std::string));
                // could load the layout and render a preview of it here
                ImGui::Text("%s", DragDropString.c_str());
            } else {
                ImGui::SetDragDropPayload("image_path", &DragDropString, sizeof(std::string));
                // could load the layout and render a preview of it here
                ImGui::Text("%s", DragDropString.c_str());
            }
            ImGui::EndDragDropSource();
        }
    });

    m_contentList.SetDisplayFilter([](std::filesystem::path const& path) {
        if (std::filesystem::is_directory(path)) {
            return true;
        } else if (!path.has_extension() || !IsSupportedExtension(path.extension().string())) {
            return false;
        }
        return true;
    });
}

void EditorPanelAssetList::Refresh() {
    auto& projectInfo = m_editorLayer.GetLayoutProject();
    m_contentList.SetPath(projectInfo.m_layoutRoot);
    m_contentList.Refresh();
}

void EditorPanelAssetList::DrawContents() {
    static char nameBuffer[1024] = { 0 };
    bool fileExistsPopup = false;

    m_deleteConfirm.Draw();

    //if (ImGui::Button("Delete Layout")) {
    //    auto const& currentSelection = m_contentList.GetCurrentSelection();
    //    if (std::filesystem::exists(currentSelection) && std::filesystem::is_regular_file(currentSelection)) {
    //        m_deleteConfirm.SetPositiveAction([this, currentSelection]() {
    //            std::filesystem::remove(currentSelection);
    //            Refresh();
    //        });
    //        m_deleteConfirm.Open();
    //    }
    //}
    m_contentList.Draw();
}
