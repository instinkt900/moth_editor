#include "common.h"
#include "editor_panel_properties.h"

#include "../properties_elements.h"
#include "../actions/editor_action.h"
#include "../imgui_ext_inspect.h"
#include "../utils.h"
#include "../editor_layer.h"

#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/layout/layout_entity_ref.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/node_rect.h"
#include "moth_ui/node_image.h"
#include "moth_ui/node_text.h"
#include "moth_ui/group.h"
#include "moth_ui/context.h"

#include "imgui-filebrowser/imfilebrowser.h"

#undef min
#undef max

namespace {
    ImGui::FileBrowser s_fileBrowser;
    std::shared_ptr<moth_ui::NodeImage> s_loadingNodeImage = nullptr;
}

EditorPanelProperties::EditorPanelProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Properties", visible, true) {
}

void EditorPanelProperties::OnLayoutLoaded() {
    m_lastSelection = nullptr;
    PropertiesInputReset();
}

void EditorPanelProperties::DrawContents() {
    std::shared_ptr<moth_ui::Node> node;

    auto& selection = m_editorLayer.GetSelection();
    if (selection.empty()) {
        node = m_editorLayer.GetRoot();
        auto entity = node->GetLayoutEntity();
        DrawNodeProperties(node);
    } else {
        auto const node = *std::begin(selection);
        auto entity = node->GetLayoutEntity();
        DrawNodeProperties(node);
    }

    if (m_lastSelection && node.get() != m_lastSelection) {
        PropertiesInputClear();
    }
    m_lastSelection = node.get();
}

void EditorPanelProperties::DrawNodeProperties(std::shared_ptr<moth_ui::Node> node, bool recurseChildren) {
    auto const entity = node->GetLayoutEntity();

    if (entity->GetType() != moth_ui::LayoutEntityType::Layout) {
        DrawCommonProperties(node);
    }

    switch (entity->GetType()) {
    case moth_ui::LayoutEntityType::Entity:
    case moth_ui::LayoutEntityType::Group:
        // shouldnt get hit
        break;
    case moth_ui::LayoutEntityType::Rect:
        DrawRectProperties(std::static_pointer_cast<moth_ui::NodeRect>(node));
        break;
    case moth_ui::LayoutEntityType::Image:
        DrawImageProperties(std::static_pointer_cast<moth_ui::NodeImage>(node));
        break;
    case moth_ui::LayoutEntityType::Text:
        DrawTextProperties(std::static_pointer_cast<moth_ui::NodeText>(node));
        break;
    case moth_ui::LayoutEntityType::Ref:
        DrawRefProperties(std::static_pointer_cast<moth_ui::Group>(node), recurseChildren);
        break;
    case moth_ui::LayoutEntityType::Layout:
        DrawLayoutProperties(std::static_pointer_cast<moth_ui::Group>(node));
        break;
    }
}

void EditorPanelProperties::DrawCommonProperties(std::shared_ptr<moth_ui::Node> node) {
    auto const entity = node->GetLayoutEntity();

    PropertiesInput(
        "ID", entity->m_id.c_str(),
        [&](auto changedValue) {
            node->SetId(changedValue);
        },
        [=](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_id, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput("Visible", node->IsVisible(), [&](bool newValue) {
        auto action = MakeChangeValueAction(entity->m_visible, entity->m_visible, newValue, [node]() { node->ReloadEntity(); });
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput("Locked", m_editorLayer.IsLocked(node), [&](bool value) {
        auto action = MakeLockAction(node, value, m_editorLayer);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput("Show Bounds", node->GetShowRect(), [&](bool value) {
        auto action = MakeShowBoundsAction(node, value);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput(
        "Bounds", node->GetLayoutRect(),
        [&](moth_ui::LayoutRect changedValue) {
            m_editorLayer.BeginEditBounds(node);
            node->GetLayoutRect() = changedValue;
            node->RecalculateBounds();
        },
        [=](moth_ui::LayoutRect oldValue, moth_ui::LayoutRect newValue) {
            m_editorLayer.EndEditBounds();
        });

    PropertiesInput(
        "Color", node->GetColor(),
        [&](moth_ui::Color changedValue) {
            m_editorLayer.BeginEditColor(node);
            node->SetColor(changedValue);
        },
        [=](moth_ui::Color oldValue, moth_ui::Color newValue) {
            m_editorLayer.EndEditColor();
        });

    PropertiesInput(
        "Blend Mode", node->GetBlendMode(),
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_blend, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawRectProperties(std::shared_ptr<moth_ui::NodeRect> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityRect>(node->GetLayoutEntity());

    PropertiesInput(
        "Filled", entity->m_filled,
        [&](auto const newValue) {
            auto const oldValue = entity->m_filled;
            auto action = MakeChangeValueAction(entity->m_filled, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawImageProperties(std::shared_ptr<moth_ui::NodeImage> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(node->GetLayoutEntity());

    PropertiesInput(
        "Image Scale Type", entity->m_imageScaleType,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_imageScaleType, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Image Scale", entity->m_imageScale,
        [&](float changedValue) {
            entity->m_imageScale = changedValue;
            node->ReloadEntity();
        },
        [=](float oldValue, float newValue) {
            auto action = MakeChangeValueAction(entity->m_imageScale, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Source Rect", entity->m_sourceRect,
        [&](auto newValue) {
            entity->m_sourceRect = newValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_sourceRect, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Target Borders", entity->m_targetBorders,
        [&](auto newValue) {
            entity->m_targetBorders = newValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_targetBorders, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Source Borders", entity->m_sourceBorders,
        [&](auto newValue) {
            entity->m_sourceBorders = newValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_sourceBorders, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    imgui_ext::Inspect("Image", node->GetImage());

    {
        using namespace moth_ui;

        FloatVec2 const previewImageMin{ ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
        FloatVec2 const previewImageMax{ ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y };
        FloatVec2 const previewImageSize = previewImageMax - previewImageMin;
        FloatVec2 const sourceImageDimensions = static_cast<FloatVec2>(node->GetImage()->GetDimensions());
        FloatVec2 const imageToPreviewFactor = previewImageSize / sourceImageDimensions;
        FloatRect const sourceRect = static_cast<FloatRect>(node->GetSourceRect());

        auto const ImageToPreview = [&](FloatVec2 const& srcVec) -> FloatVec2 {
            return srcVec * imageToPreviewFactor;
        };

        FloatVec2 const srcMin = previewImageMin + ImageToPreview(sourceRect.topLeft);
        FloatVec2 const srcMax = previewImageMin + ImageToPreview(sourceRect.bottomRight);

        auto drawList = ImGui::GetWindowDrawList();

        // source rect preview
        drawList->AddRect(ImVec2{ srcMin.x, srcMin.y }, ImVec2{ srcMax.x, srcMax.y }, 0xFF00FFFF);

        // 9 slice preview
        if (node->GetImageScaleType() == moth_ui::ImageScaleType::NineSlice) {
            FloatVec2 const slice1 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(node->GetSourceSlices()[1]));
            FloatVec2 const slice2 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(node->GetSourceSlices()[2]));

            drawList->AddLine(ImVec2{ slice1.x, srcMin.y }, ImVec2{ slice1.x, srcMax.y }, 0xFF004477);
            drawList->AddLine(ImVec2{ slice2.x, srcMin.y }, ImVec2{ slice2.x, srcMax.y }, 0xFF004477);
            drawList->AddLine(ImVec2{ srcMin.x, slice1.y }, ImVec2{ srcMax.x, slice1.y }, 0xFF004477);
            drawList->AddLine(ImVec2{ srcMin.x, slice2.y }, ImVec2{ srcMax.x, slice2.y }, 0xFF004477);
        }
    }

    if (ImGui::Button("Load Image..")) {
        s_fileBrowser.SetTitle("Load Image..");
        s_fileBrowser.SetTypeFilters({ ".jpg", ".jpeg", ".png", ".bmp" });
        s_fileBrowser.Open();
        s_loadingNodeImage = node;
    }

    if (s_loadingNodeImage) {
        s_fileBrowser.Display();
        if (s_fileBrowser.HasSelected()) {
            auto const targetImageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(s_loadingNodeImage->GetLayoutEntity());
            auto const oldPath = targetImageEntity->m_imagePath;
            auto const newPath = s_fileBrowser.GetSelected();
            auto action = MakeChangeValueAction(entity->m_imagePath, oldPath, newPath, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
            s_fileBrowser.ClearSelected();
            s_loadingNodeImage = nullptr;
        }
    }
}

void EditorPanelProperties::DrawTextProperties(std::shared_ptr<moth_ui::NodeText> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityText>(node->GetLayoutEntity());

    PropertiesInput(
        "Font Size", entity->m_fontSize,
        [&](int changedValue) {
            entity->m_fontSize = changedValue;
            node->ReloadEntity();
        },
        [=](int oldValue, int newValue) {
            auto action = MakeChangeValueAction(entity->m_fontSize, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    auto const fontNames = moth_ui::Context::GetCurrentContext()->GetFontFactory().GetFontNameList();
    PropertiesInput(
        "Font", fontNames, entity->m_fontName,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_fontName, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "H Alignment", entity->m_horizontalAlignment,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_horizontalAlignment, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "V Alignment", entity->m_verticalAlignment,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_verticalAlignment, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Drop Shadow", entity->m_dropShadow,
        [&](auto newValue) {
            auto const oldValue = entity->m_dropShadow;
            auto action = MakeChangeValueAction(entity->m_dropShadow, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Drop Shadow Offset", entity->m_dropShadowOffset,
        [&](auto changedValue) {
            entity->m_dropShadowOffset = changedValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_dropShadowOffset, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Drop Shadow Color", entity->m_dropShadowColor,
        [&](auto changedValue) {
            entity->m_dropShadowColor = changedValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_dropShadowColor, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Text", entity->m_text.c_str(), 8,
        [&](char const* changedValue) {
            node->SetText(changedValue);
        },
        [=](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_text, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

char const* GetChildName(std::shared_ptr<moth_ui::LayoutEntity> entity) {
    switch (entity->GetType()) {
    case moth_ui::LayoutEntityType::Entity:
        return "Entity";
    case moth_ui::LayoutEntityType::Group:
        return "Group";
        break;
    case moth_ui::LayoutEntityType::Rect:
        return "Rect";
    case moth_ui::LayoutEntityType::Image:
        return "Image";
    case moth_ui::LayoutEntityType::Text:
        return "Text";
    case moth_ui::LayoutEntityType::Ref:
        return "Ref";
    }
    return "Unknown";
}

void EditorPanelProperties::DrawRefProperties(std::shared_ptr<moth_ui::Group> node, bool recurseChildren) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityRef>(node->GetLayoutEntity());

    if (recurseChildren) {
        if (ImGui::TreeNode("Children")) {
            int childIndex = 0;
            for (auto&& child : node->GetChildren()) {
                static std::string name;
                name = fmt::format("{}: {}", childIndex, GetChildName(child->GetLayoutEntity()));
                if (ImGui::TreeNode(name.c_str())) {
                    DrawNodeProperties(child, false);
                    ImGui::TreePop();
                }
                ++childIndex;
            }
            ImGui::TreePop();
        }
    }
}

void EditorPanelProperties::DrawLayoutProperties(std::shared_ptr<moth_ui::Group> node) {
    auto const entity = std::static_pointer_cast<moth_ui::Layout>(node->GetLayoutEntity());

    PropertiesInput(
        "Class", entity->m_class.c_str(), 
        [&](std::string changedValue) {
            entity->m_class = changedValue;
        },
        [=](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_class, oldValue, newValue, nullptr);
            m_editorLayer.PerformEditAction(std::move(action));
        });
}
