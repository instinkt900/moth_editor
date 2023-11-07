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

#include <nfd.h>

namespace {
    std::vector<std::pair<char const*, std::function<void(EditorLayer&)>>> ElementButtons = {
        {
            "Rect",
            [](EditorLayer& editorLayer) { AddEntity<moth_ui::LayoutEntityRect>(editorLayer); },
        },
        {
            "Image",
            [](EditorLayer& editorLayer) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_OpenDialog("jpg,jpeg,png,bmp", currentPath.c_str(), &outPath);

                if (result == NFD_OKAY) {
                    std::filesystem::path filePath = outPath;
                    moth_ui::LayoutRect bounds;
                    bounds.anchor.topLeft = { 0, 0 };
                    bounds.anchor.bottomRight = { 0, 0 };
                    bounds.offset.topLeft = { 0, 0 };
                    bounds.offset.bottomRight = { 100, 100 };
                    AddEntityWithBounds<moth_ui::LayoutEntityImage>(editorLayer, bounds, filePath);
                }
            },
        },
        {
            "Sublayout",
            [](EditorLayer& editorLayer) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_OpenDialog(moth_ui::Layout::Extension.c_str(), currentPath.c_str(), &outPath);

                if (result == NFD_OKAY) {
                    std::filesystem::path filePath = outPath;
                    std::shared_ptr<moth_ui::Layout> referencedLayout;
                    auto const loadResult = moth_ui::Layout::Load(filePath, &referencedLayout);
                    if (loadResult == moth_ui::Layout::LoadResult::Success) {
                        moth_ui::LayoutRect bounds;
                        bounds.anchor.topLeft = { 0, 0 };
                        bounds.anchor.bottomRight = { 0, 0 };
                        bounds.offset.topLeft = { 0, 0 };
                        bounds.offset.bottomRight = { 100, 100 };
                        AddEntityWithBounds<moth_ui::LayoutEntityRef>(editorLayer, bounds, *referencedLayout);
                    } else {
                        if (loadResult == moth_ui::Layout::LoadResult::DoesNotExist) {
                            editorLayer.ShowError("File not found.");
                        } else if (loadResult == moth_ui::Layout::LoadResult::IncorrectFormat) {
                            editorLayer.ShowError("File was not valid.");
                        }
                    }
                }
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
    ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
    ImGui::GetWindowPos();
    ImGui::GetWindowContentRegionMin();
    for (auto& [label, func] : ElementButtons) {
        if (ImGui::Button(label, button_size)) {
            func(m_editorLayer);
        }
        if ((ImGui::GetItemRectMax().x + button_size.x) < (ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)) {
            ImGui::SameLine();
        }
    }
}
