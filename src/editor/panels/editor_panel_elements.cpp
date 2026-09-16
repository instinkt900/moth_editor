#include "common.h"
#include "editor_panel_elements.h"
#include "../editor_layer.h"
#include "moth/ui/layout/layout_entity_rect.h"
#include "moth/ui/layout/layout_entity_image.h"
#include "moth/ui/layout/layout_entity_ref.h"
#include "moth/ui/layout/layout_entity_clip.h"
#include "moth/ui/layout/layout_entity_text.h"
#include "moth/ui/layout/layout_entity_flipbook.h"
#include "moth/ui/layout/layout_entity_gradient.h"
#include "moth/ui/layout/layout.h"
#include "moth/ui/nodes/group.h"
#include "../element_utils.h"

#include <nfd.h>

namespace {
    std::vector<std::pair<char const*, std::function<void(EditorLayer&)>>> ElementButtons = {
        {
            "Rect",
            [](EditorLayer& editorLayer) { AddEntity<moth::ui::LayoutEntityRect>(editorLayer); },
        },
        {
            "Image",
            [](EditorLayer& editorLayer) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_OpenDialog("jpg,jpeg,png,bmp", currentPath.c_str(), &outPath);

                if (result == NFD_OKAY) {
                    std::filesystem::path filePath = outPath;
                    NFD_Free(outPath);
                    moth::ui::LayoutRect bounds;
                    bounds.anchor.topLeft = { 0, 0 };
                    bounds.anchor.bottomRight = { 0, 0 };
                    bounds.offset.topLeft = { 0, 0 };
                    bounds.offset.bottomRight = { 100, 100 };
                    AddEntityWithBounds<moth::ui::LayoutEntityImage>(
                        editorLayer, bounds,
                        filePath);
                }
            },
        },
        {
            "Sublayout",
            [](EditorLayer& editorLayer) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_OpenDialog(moth::ui::Layout::Extension.c_str(), currentPath.c_str(), &outPath);

                if (result == NFD_OKAY) {
                    std::filesystem::path filePath = outPath;
                    NFD_Free(outPath);
                    auto [referencedLayout, loadResult] = moth::ui::Layout::Load(filePath);
                    if (loadResult == moth::ui::Layout::LoadResult::Success) {
                        moth::ui::LayoutRect bounds;
                        bounds.anchor.topLeft = { 0, 0 };
                        bounds.anchor.bottomRight = { 0, 0 };
                        bounds.offset.topLeft = { 0, 0 };
                        bounds.offset.bottomRight = { 100, 100 };
                        AddEntityWithBounds<moth::ui::LayoutEntityRef>(editorLayer, bounds, *referencedLayout);
                    } else {
                        if (loadResult == moth::ui::Layout::LoadResult::DoesNotExist) {
                            editorLayer.ShowError("File not found.");
                        } else if (loadResult == moth::ui::Layout::LoadResult::IncorrectFormat) {
                            editorLayer.ShowError("File was not valid.");
                        }
                    }
                }
            },
        },
        {
            "Text",
            [](EditorLayer& editorLayer) { AddEntity<moth::ui::LayoutEntityText>(editorLayer); },
        },
        {
            "Clip Rect",
            [](EditorLayer& editorLayer) { AddEntity<moth::ui::LayoutEntityClip>(editorLayer); },
        },
        {
            "Gradient",
            [](EditorLayer& editorLayer) { AddEntity<moth::ui::LayoutEntityGradient>(editorLayer); },
        },
        {
            "Flipbook",
            [](EditorLayer& editorLayer) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_OpenDialog("json", currentPath.c_str(), &outPath);

                if (result == NFD_OKAY) {
                    std::filesystem::path filePath = outPath;
                    NFD_Free(outPath);
                    AddEntity<moth::ui::LayoutEntityFlipbook>(editorLayer, filePath);
                }
            },
        },
    };
}

EditorPanelElements::EditorPanelElements(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Elements", visible, true) {
}

void EditorPanelElements::DrawContents() {
    ImVec2 const buttonSize(ImGui::GetFontSize() * 7.0f, 0.0f);
    for (auto& [label, func] : ElementButtons) {
        if (ImGui::Button(label, buttonSize)) {
            func(m_editorLayer);
        }
        if ((ImGui::GetItemRectMax().x + buttonSize.x) < (ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x)) {
            ImGui::SameLine();
        }
    }
}
