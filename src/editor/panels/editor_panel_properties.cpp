#include "common.h"
#include "editor_panel_properties.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/layout/layout_entity_9slice.h"
#include "moth_ui/utils/imgui_ext_inspect.h"
#include "moth_ui/node_rect.h"
#include "moth_ui/node_image.h"
#include "moth_ui/node_text.h"
#include "moth_ui/node_9slice.h"
#include "../utils.h"
#include "moth_ui/context.h"
#include "../properties_elements.h"
#include "../actions/editor_action.h"

namespace {
    ImGui::FileBrowser s_fileBrowser;
    std::shared_ptr<moth_ui::NodeImage> s_loadingNodeImage = nullptr;
    std::shared_ptr<moth_ui::Node9Slice> s_loadingNode9Slice = nullptr;
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
        case moth_ui::LayoutEntityType::NineSlice:
            Draw9SliceProperties();
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

    imgui_ext::Inspect("Image", imageNode->GetImage());

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

void EditorPanelProperties::Draw9SliceProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const sliceNode = std::static_pointer_cast<moth_ui::Node9Slice>(selection);
    auto const sliceEntity = std::static_pointer_cast<moth_ui::LayoutEntity9Slice>(selection->GetLayoutEntity());

    PropertiesInput(
        "Image Scale Type", sliceEntity->m_imageScaleType,
        [&](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(sliceEntity->m_imageScaleType, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Target Borders", sliceEntity->m_targetBorder,
        [&](auto newValue) {
            sliceEntity->m_targetBorder = newValue;
            selection->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(sliceEntity->m_targetBorder, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    PropertiesInput(
        "Source Borders", sliceEntity->m_srcBorder,
        [&](auto newValue) {
            sliceEntity->m_srcBorder = newValue;
            selection->ReloadEntity();
        },
        [=](auto oldValue, auto newValue) {
            auto action = MakeChangeValueAction(sliceEntity->m_srcBorder, oldValue, newValue, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
        });

    imgui_ext::Inspect("Image", sliceNode->GetImage());

    if (ImGui::Button("Load Image..")) {
        s_fileBrowser.SetTitle("Load Image..");
        s_fileBrowser.SetTypeFilters({ ".jpg", ".jpeg", ".png", ".bmp" });
        s_fileBrowser.Open();
        s_loadingNode9Slice = sliceNode;
    }

    if (s_loadingNode9Slice) {
        s_fileBrowser.Display();
        if (s_fileBrowser.HasSelected()) {
            auto const target9SliceEntity = std::static_pointer_cast<moth_ui::LayoutEntity9Slice>(s_loadingNode9Slice->GetLayoutEntity());
            auto const oldPath = target9SliceEntity->m_imagePath;
            auto const newPath = s_fileBrowser.GetSelected().string();
            auto action = MakeChangeValueAction(sliceEntity->m_imagePath, oldPath, newPath, [selection]() { selection->ReloadEntity(); });
            m_editorLayer.PerformEditAction(std::move(action));
            s_fileBrowser.ClearSelected();
            s_loadingNode9Slice = nullptr;
        }
    }
}