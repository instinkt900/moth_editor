#include "common.h"
#include "editor_panel_properties.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/utils/imgui_ext_inspect.h"
#include "moth_ui/node_rect.h"
#include "moth_ui/node_image.h"
#include "moth_ui/node_text.h"
#include "../utils.h"
#include "moth_ui/context.h"
#include "../properties_elements.h"
#include "../actions/editor_action.h"

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

void EditorPanelProperties::DrawContents() {
    if (auto selection = m_editorLayer.GetSelection()) {
        auto entity = selection->GetLayoutEntity();
        imgui_ext::FocusGroupBegin(&m_focusContext);
        DrawEntityProperties();
        switch (entity->GetType()) {
        case moth_ui::LayoutEntityType::Entity:
            break;
        case moth_ui::LayoutEntityType::Group:
            break;
        case moth_ui::LayoutEntityType::Rect:
            DrawRectProperties();
            break;
        case moth_ui::LayoutEntityType::Image:
            DrawImageProperties();
            break;
        case moth_ui::LayoutEntityType::Text:
            DrawTextProperties();
            break;
        }
        imgui_ext::FocusGroupEnd();
    } else {
        PropertiesInputClear();
    }
}

void EditorPanelProperties::DrawEntityProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const entity = selection->GetLayoutEntity();

    PropertiesInput(
        "ID", entity->m_id.c_str(),
        [&](auto changedValue) {
            selection->SetId(changedValue);
        },
        [=](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(entity->m_id, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput("Visible", selection->IsVisible(), [&](bool value) {
        auto action = MakeVisibilityAction(selection, value);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput("Show Bounds", selection->GetShowRect(), [&](bool value) {
        auto action = MakeShowBoundsAction(selection, value);
        m_editorLayer.PerformEditAction(std::move(action));
    });

    PropertiesInput(
        "Bounds", selection->GetLayoutRect(),
        [&](moth_ui::LayoutRect changedValue) {
            m_editorLayer.BeginEditBounds();
            selection->GetLayoutRect() = changedValue;
            selection->RecalculateBounds();
        },
        [=](moth_ui::LayoutRect oldValue, moth_ui::LayoutRect newValue) {
            m_editorLayer.EndEditBounds();
        });

    PropertiesInput(
        "Color", selection->GetColor(),
        [&](moth_ui::Color changedValue) {
            m_editorLayer.BeginEditColor();
            selection->SetColor(changedValue);
        },
        [=](moth_ui::Color oldValue, moth_ui::Color newValue) {
            m_editorLayer.EndEditColor();
        });

    PropertiesInput(
        "Blend Mode", selection->GetBlendMode(),
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(entity->m_blend, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawRectProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const rectNode = std::static_pointer_cast<moth_ui::NodeRect>(selection);
    auto const rectEntity = std::static_pointer_cast<moth_ui::LayoutEntityRect>(selection->GetLayoutEntity());

    PropertiesInput(
        "Filled", rectEntity->m_filled,
        [&](auto const newValue) {
            auto const oldValue = rectEntity->m_filled;
            auto action = MakeChangeValueAction(rectEntity->m_filled, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}

void EditorPanelProperties::DrawImageProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const imageNode = std::static_pointer_cast<moth_ui::NodeImage>(selection);
    auto const imageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(selection->GetLayoutEntity());

    PropertiesInput(
        "Image Scale Type", imageEntity->m_imageScaleType,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(imageEntity->m_imageScaleType, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Image Scale", imageEntity->m_imageScale,
        [&](float changedValue) {
            imageEntity->m_imageScale = changedValue;
            selection->ReloadEntity();
        },
        [=](float oldValue, float newValue) {
            auto action = MakeChangeValueAction(imageEntity->m_imageScale, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Source Rect", imageEntity->m_sourceRect,
        [&](auto newValue) {
            imageEntity->m_sourceRect = newValue;
            selection->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(imageEntity->m_sourceRect, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Target Borders", imageEntity->m_targetBorders,
        [&](auto newValue) {
            imageEntity->m_targetBorders = newValue;
            selection->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(imageEntity->m_targetBorders, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Source Borders", imageEntity->m_sourceBorders,
        [&](auto newValue) {
            imageEntity->m_sourceBorders = newValue;
            selection->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(imageEntity->m_sourceBorders, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    imgui_ext::Inspect("Image", imageNode->GetImage());

    {
        using namespace moth_ui;

        FloatVec2 const previewImageMin{ ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y };
        FloatVec2 const previewImageMax{ ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y };
        FloatVec2 const previewImageSize = previewImageMax - previewImageMin;
        FloatVec2 const sourceImageDimensions = static_cast<FloatVec2>(imageNode->GetImage()->GetDimensions());
        FloatVec2 const imageToPreviewFactor = previewImageSize / sourceImageDimensions;
        FloatRect const sourceRect = static_cast<FloatRect>(imageNode->GetSourceRect());

        auto const ImageToPreview = [&](FloatVec2 const& srcVec) -> FloatVec2 {
            return srcVec * imageToPreviewFactor;
        };

        FloatVec2 const srcMin = previewImageMin + ImageToPreview(sourceRect.topLeft);
        FloatVec2 const srcMax = previewImageMin + ImageToPreview(sourceRect.bottomRight);

        auto drawList = ImGui::GetWindowDrawList();

        // source rect preview
        drawList->AddRect(ImVec2{ srcMin.x, srcMin.y }, ImVec2{ srcMax.x, srcMax.y }, 0xFF00FFFF);

        // 9 slice preview
        if (imageNode->GetImageScaleType() == moth_ui::ImageScaleType::NineSlice) {
            FloatVec2 const slice1 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(imageNode->GetSourceSlices()[1]));
            FloatVec2 const slice2 = previewImageMin + ImageToPreview(static_cast<FloatVec2>(imageNode->GetSourceSlices()[2]));

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
        s_loadingNodeImage = imageNode;
    }

    if (s_loadingNodeImage) {
        s_fileBrowser.Display();
        if (s_fileBrowser.HasSelected()) {
            auto const targetImageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(s_loadingNodeImage->GetLayoutEntity());
            auto const oldPath = targetImageEntity->m_imagePath;
            auto const newPath = s_fileBrowser.GetSelected().string();
            auto action = MakeChangeValueAction(imageEntity->m_imagePath, oldPath, newPath, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
            s_fileBrowser.ClearSelected();
            s_loadingNodeImage = nullptr;
        }
    }
}

void EditorPanelProperties::DrawTextProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const textNode = std::static_pointer_cast<moth_ui::NodeText>(selection);
    auto const textEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(selection->GetLayoutEntity());

    PropertiesInput(
        "Font Size", textEntity->m_fontSize,
        [&](int changedValue) {
            textEntity->m_fontSize = changedValue;
            textNode->ReloadEntity();
        },
        [=](int oldValue, int newValue) {
            auto action = MakeChangeValueAction(textEntity->m_fontSize, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    auto const fontNames = moth_ui::Context::GetCurrentContext().GetFontFactory().GetFontNameList();
    PropertiesInput(
        "Font", fontNames, textEntity->m_fontName,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(textEntity->m_fontName, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "H Alignment", textEntity->m_horizontalAlignment,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(textEntity->m_horizontalAlignment, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "V Alignment", textEntity->m_verticalAlignment,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(textEntity->m_verticalAlignment, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Text", textEntity->m_text.c_str(), 8,
        [&](char const* changedValue) {
            textNode->SetText(changedValue);
        },
        [=](std::string oldValue, std::string newValue) {
            auto action = MakeChangeValueAction(textEntity->m_text, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });
}
