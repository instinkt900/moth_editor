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
    : EditorPanel(editorLayer, "Image List", visible, true) {
}

void EditorPanelImageList::Refresh() {
    m_selectedIndex = -1;
    m_imageList.clear();
    auto& projectInfo = m_editorLayer.GetLayoutProject();
    for (auto& entry : std::filesystem::directory_iterator(projectInfo.m_imageRoot)) {
        auto const& filePath = entry.path();
        if (filePath.has_extension()) {
            auto const fileExtension = filePath.extension().string();
            if (IsSupportedExtension(fileExtension)) {
                ImageInfo info{
                    filePath.string(),
                    filePath.filename().string()
                };
                m_imageList.push_back(info);
            }
        }
    }
}

void EditorPanelImageList::DrawContents() {
    ImGui::BeginListBox("##image_list", ImVec2(-FLT_MIN, -FLT_MIN));
    for (int i = 0; i < m_imageList.size(); ++i) {
        auto& imageInfo = m_imageList[i];
        bool selected = m_selectedIndex == i;
        if (ImGui::Selectable(imageInfo.m_name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
            m_selectedIndex = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                m_editorLayer.AddImage(imageInfo.m_path.c_str());
            }
        }
    }
    ImGui::EndListBox();
}
