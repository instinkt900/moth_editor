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

#include <nfd.h>

#undef min
#undef max

EditorPanelProperties::EditorPanelProperties(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Properties", visible, true) {
}

void EditorPanelProperties::OnLayoutLoaded() {
    m_lastSelection = nullptr;
}

bool EditorPanelProperties::BeginPanel() {
    auto& selection = m_editorLayer.GetSelection();
    if (selection.empty()) {
        m_currentSelection = m_editorLayer.GetRoot();
    } else {
        m_currentSelection = *std::begin(selection);
    }

    BeginEdits();
    bool ret = EditorPanel::BeginPanel();
    ImGui::PushID(m_currentSelection.get());
    return ret;
}

void EditorPanelProperties::EndPanel() {
    EndEdits();

    if (m_lastSelection && m_currentSelection != m_lastSelection) {
        CommitEditContext();
    }
    m_lastSelection = m_currentSelection;

    ImGui::PopID();
    EditorPanel::EndPanel();
}

void EditorPanelProperties::DrawContents() {
    if (m_currentSelection) {
        DrawNodeProperties(m_currentSelection);
    }
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
    default:
        break;
    }
}

void EditorPanelProperties::DrawCommonProperties(std::shared_ptr<moth_ui::Node> node) {
    auto const entity = node->GetLayoutEntity();

    PropertiesInput<char const*>(
        "ID", entity->m_id.c_str(),
        [&](char const* changedValue) {
            node->SetId(changedValue);
        },
        [=](char const* oldValue, char const* newValue) {
            auto action = MakeChangeValueAction(entity->m_id, std::string(oldValue), std::string(newValue), [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<bool>("Visible", node->IsVisible(), [&](bool newValue) {
        auto action = MakeChangeValueAction(entity->m_visible, entity->m_visible, newValue, [node]() { node->ReloadEntity(); });
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput<bool>("Locked", m_editorLayer.IsLocked(node), [&](bool value) {
        auto action = MakeLockAction(node, value, m_editorLayer);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput<bool>("Show Bounds", node->GetShowRect(), [&](bool value) {
        auto action = MakeShowBoundsAction(node, value);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput<moth_ui::LayoutRect>(
        "Bounds", node->GetLayoutRect(),
        [&](moth_ui::LayoutRect changedValue) {
            m_editorLayer.BeginEditBounds(node);
            node->GetLayoutRect() = changedValue;
            node->RecalculateBounds();
        },
        [=](moth_ui::LayoutRect oldValue, moth_ui::LayoutRect newValue) {
            m_editorLayer.EndEditBounds();
        });

    PropertiesInput<moth_ui::Color>(
        "Color", node->GetColor(),
        [&](moth_ui::Color changedValue) {
            m_editorLayer.BeginEditColor(node);
            node->SetColor(changedValue);
        },
        [=](moth_ui::Color oldValue, moth_ui::Color newValue) {
            m_editorLayer.EndEditColor();
        });

    PropertiesInput<moth_ui::BlendMode>(
        "Blend Mode", node->GetBlendMode(), {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_blend, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawRectProperties(std::shared_ptr<moth_ui::NodeRect> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityRect>(node->GetLayoutEntity());

    PropertiesInput<bool>(
        "Filled", entity->m_filled,
        [&](auto const newValue) {
            auto const oldValue = entity->m_filled;
            auto action = MakeChangeValueAction(entity->m_filled, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawImageProperties(std::shared_ptr<moth_ui::NodeImage> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(node->GetLayoutEntity());

    PropertiesInput<moth_ui::ImageScaleType>(
        "Image Scale Type", entity->m_imageScaleType, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_imageScaleType, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<float>(
        "Image Scale", entity->m_imageScale,
        [&](float changedValue) {
            entity->m_imageScale = changedValue;
            node->ReloadEntity();
        },
        [=](float oldValue, float newValue) {
            auto action = MakeChangeValueAction(entity->m_imageScale, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::IntRect>(
        "Source Rect", entity->m_sourceRect,
        [&](auto newValue) {
            entity->m_sourceRect = newValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_sourceRect, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::LayoutRect>(
        "Target Borders", entity->m_targetBorders,
        [&](auto newValue) {
            entity->m_targetBorders = newValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_targetBorders, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::IntRect>(
        "Source Borders", entity->m_sourceBorders,
        [&](auto newValue) {
            entity->m_sourceBorders = newValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_sourceBorders, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    imgui_ext::Image(node->GetImage(), 200, 200);

    if (node->GetImage()) {
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
        auto const rectColor = moth_ui::ToABGR(m_editorLayer.GetConfig().PreviewSourceRectColor);
        drawList->AddRect(ImVec2{ srcMin.x, srcMin.y }, ImVec2{ srcMax.x, srcMax.y }, rectColor);

        // 9 slice preview
        auto const sliceColor = moth_ui::ToABGR(m_editorLayer.GetConfig().PreviewImageSliceColor);
        if (node->GetImageScaleType() == moth_ui::ImageScaleType::NineSlice) {
            FloatVec2 const slice1 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(node->GetSourceSlices()[1]));
            FloatVec2 const slice2 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(node->GetSourceSlices()[2]));

            drawList->AddLine(ImVec2{ slice1.x, srcMin.y }, ImVec2{ slice1.x, srcMax.y }, sliceColor);
            drawList->AddLine(ImVec2{ slice2.x, srcMin.y }, ImVec2{ slice2.x, srcMax.y }, sliceColor);
            drawList->AddLine(ImVec2{ srcMin.x, slice1.y }, ImVec2{ srcMax.x, slice1.y }, sliceColor);
            drawList->AddLine(ImVec2{ srcMin.x, slice2.y }, ImVec2{ srcMax.x, slice2.y }, sliceColor);
        }
    }

    if (ImGui::Button("Load Image..")) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog("jpg, jpeg, png, bmp", currentPath.c_str(), &outPath);

        if (result == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            auto const targetImageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(node->GetLayoutEntity());
            auto const oldPath = targetImageEntity->m_imagePath;
            auto const newPath = filePath;
            auto action = MakeChangeValueAction(entity->m_imagePath, oldPath, newPath, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        }
    }
}

void EditorPanelProperties::DrawTextProperties(std::shared_ptr<moth_ui::NodeText> node) {
    auto const entity = std::static_pointer_cast<moth_ui::LayoutEntityText>(node->GetLayoutEntity());

    PropertiesInput<int>(
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
    PropertiesInputList(
        "Font", fontNames, entity->m_fontName,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_fontName, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::TextHorizAlignment>(
        "H Alignment", entity->m_horizontalAlignment, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_horizontalAlignment, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::TextVertAlignment>(
        "V Alignment", entity->m_verticalAlignment, {},
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_verticalAlignment, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<bool>(
        "Drop Shadow", entity->m_dropShadow,
        [&](auto newValue) {
            auto const oldValue = entity->m_dropShadow;
            auto action = MakeChangeValueAction(entity->m_dropShadow, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::IntVec2>(
        "Drop Shadow Offset", entity->m_dropShadowOffset,
        [&](auto changedValue) {
            entity->m_dropShadowOffset = changedValue;
            node->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_dropShadowOffset, oldValue, newValue, [node]() { node->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput<moth_ui::Color>(
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
        "Text", node->GetText().c_str(), 8,
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
    default:
        return "Unknown";
    }
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

    PropertiesInput<char const*>(
        "Class", entity->m_class.c_str(),
        [&](std::string changedValue) {
            entity->m_class = changedValue;
        },
        [=](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_class, oldValue, newValue, nullptr);
            m_editorLayer.PerformEditAction(std::move(action));
        });
}
