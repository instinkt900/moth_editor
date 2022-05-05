#include "common.h"
#include "editor_panel_elements.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_ref.h"
#include "moth_ui/layout/layout_entity_clip.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/group.h"
#include "../element_utils.h"

#include "imgui-filebrowser/imfilebrowser.h"

namespace {
    enum class FileOpenMode {
        Unknown,
        SubLayout,
        Image,
        NineSlice,
    };

    ImGui::FileBrowser s_fileBrowser;
    FileOpenMode s_fileOpenMode = FileOpenMode::Unknown;

    std::vector<std::pair<char const*, std::function<void(EditorLayer&)>>> ElementButtons = {
        {
            "Rect",
            [](EditorLayer& editorLayer) { AddEntity<moth_ui::LayoutEntityRect>(editorLayer); },
        },
        {
            "Image",
            [](EditorLayer& editorLayer) {
                s_fileBrowser.SetTitle("Open..");
                s_fileBrowser.SetTypeFilters({ ".jpg", ".jpeg", ".png", ".bmp" });
                s_fileBrowser.SetPwd();
                s_fileBrowser.Open();
                s_fileOpenMode = FileOpenMode::Image;
            },
        },
        {
            "Sublayout",
            [](EditorLayer& editorLayer) {
                s_fileBrowser.SetTitle("Open..");
                s_fileBrowser.SetTypeFilters({ moth_ui::Layout::Extension });
                s_fileBrowser.SetPwd();
                s_fileBrowser.Open();
                s_fileOpenMode = FileOpenMode::SubLayout;
            },
        },
        {
            "Text",
            [](EditorLayer& editorLayer) { AddEntity<moth_ui::LayoutEntityText>(editorLayer); },
        },
        {
            "Clip Rect",
            [](EditorLayer& editorLayer) { AddEntity<moth_ui::LayoutEntityClip>(editorLayer); },
        },
    };
}

EditorPanelElements::EditorPanelElements(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Elements", visible, true) {
}

void EditorPanelElements::DrawContents() {

    s_fileBrowser.Display();
    if (s_fileBrowser.HasSelected()) {
        if (s_fileOpenMode == FileOpenMode::SubLayout) {
            std::shared_ptr<moth_ui::Layout> referencedLayout;
            auto const loadResult = moth_ui::Layout::Load(s_fileBrowser.GetSelected().string().c_str(), &referencedLayout);
            if (loadResult == moth_ui::Layout::LoadResult::Success) {
                moth_ui::LayoutRect bounds;
                bounds.anchor.topLeft = { 0, 0 };
                bounds.anchor.bottomRight = { 0, 0 };
                bounds.offset.topLeft = { 0, 0 };
                bounds.offset.bottomRight = { 100, 100 };
                AddEntityWithBounds<moth_ui::LayoutEntityRef>(m_editorLayer, bounds, *referencedLayout);
            } else {
                if (loadResult == moth_ui::Layout::LoadResult::DoesNotExist) {
                    m_editorLayer.ShowError("File not found.");
                } else if (loadResult == moth_ui::Layout::LoadResult::IncorrectFormat) {
                    m_editorLayer.ShowError("File was not valid.");
                }
            }
            s_fileBrowser.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::Image) {
            moth_ui::LayoutRect bounds;
            bounds.anchor.topLeft = { 0, 0 };
            bounds.anchor.bottomRight = { 0, 0 };
            bounds.offset.topLeft = { 0, 0 };
            bounds.offset.bottomRight = { 100, 100 };
            AddEntityWithBounds<moth_ui::LayoutEntityImage>(m_editorLayer, bounds, s_fileBrowser.GetSelected().string().c_str());
            s_fileBrowser.ClearSelected();
        }
    }

    ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
    ImGui::GetWindowPos();
    ImGui::GetWindowContentRegionMin();
    int curXPos = 0;
    for (auto& [label, func] : ElementButtons) {
        if (ImGui::Button(label, button_size)) {
            func(m_editorLayer);
        }
        if ((ImGui::GetItemRectMax().x + button_size.x) < (ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)) {
            ImGui::SameLine();
        } else {
            curXPos = 0;
        }
    }
}
