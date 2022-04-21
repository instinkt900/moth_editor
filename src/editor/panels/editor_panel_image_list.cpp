#include "common.h"
#include "editor_panel_image_list.h"
#include "../editor_layer.h"

namespace {
    static std::vector<std::string> const s_supportedExtensions{
        ".jpg",
        ".jpeg",
        ".png"
    };
    bool IsSupportedExtension(std::string const& ext) {
        return std::end(s_supportedExtensions) != ranges::find(s_supportedExtensions, ext);
    }
}

EditorPanelImageList::EditorPanelImageList(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Image List", visible, true)
    , m_contentList({}) {
    m_contentList.SetDisplayNameAction([](std::filesystem::path const& path) {
        return path.filename().string();
    });

    m_contentList.SetDoubleClickAction([this](std::filesystem::path const& path) {
        m_editorLayer.AddImage(path.string().c_str());
    });

    m_contentList.SetChangeDirectoryAction([this](std::filesystem::path const& path) {
        auto& project = m_editorLayer.GetLayoutProject();
        project.m_imageRoot = path.string();
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

void EditorPanelImageList::Refresh() {
    auto& project = m_editorLayer.GetLayoutProject();
    m_contentList.SetPath(project.m_imageRoot);
    m_contentList.Refresh();
}

void EditorPanelImageList::DrawContents() {
    m_contentList.Draw();
}
