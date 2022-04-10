#include "common.h"
#include "properties_editor.h"
#include "editor_layer.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/utils/imgui_ext_inspect.h"
#include "moth_ui/node_image.h"
#include "moth_ui/node_text.h"
#include "utils.h"
#include "moth_ui/context.h"

namespace {
    ImGui::FileBrowser s_fileBrowser;
    std::shared_ptr<moth_ui::NodeImage> s_loadingNodeImage = nullptr;
}

PropertiesEditor::PropertiesEditor(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

PropertiesEditor::~PropertiesEditor() {
}

void PropertiesEditor::Draw() {
    if (auto selection = m_editorLayer.GetSelection()) {
        auto entity = selection->GetLayoutEntity();
        imgui_ext::FocusGroupBegin(&m_focusContext);
        DrawEntityProperties();
        switch (entity->GetType()) {
        case moth_ui::LayoutEntityType::Entity:
            break;
        case moth_ui::LayoutEntityType::Group:
            DrawGroupProperties();
            break;
        case moth_ui::LayoutEntityType::Image:
            DrawImageProperties();
            break;
        case moth_ui::LayoutEntityType::Text:
            DrawTextProperties();
            break;
        }
        imgui_ext::FocusGroupEnd();
    }
}

void PropertiesEditor::DrawEntityProperties() {
    int const frame = m_editorLayer.GetSelectedFrame();
    auto selection = m_editorLayer.GetSelection();
    auto entity = selection->GetLayoutEntity();

    imgui_ext::FocusGroupInputText(
        "ID", entity->GetId(),
        [&](std::string const& value) { entity->SetId(value); },
        [&]() {});

    bool visible = selection->IsVisible();
    ImGui::Checkbox("Visible", &visible);
    if (visible != selection->IsVisible()) {
        selection->SetVisible(visible);
    }

    bool showRect = selection->GetShowRect();
    ImGui::Checkbox("Show Rect", &showRect);
    if (showRect != selection->GetShowRect()) {
        selection->SetShowRect(showRect);
    }

    imgui_ext::FocusGroupInputLayoutRect(
        "Bounds", selection->GetLayoutRect(),
        [&](moth_ui::LayoutRect const& value) { m_editorLayer.BeginEditBounds(); selection->GetLayoutRect() = value; selection->RecalculateBounds(); },
        [&]() { m_editorLayer.EndEditBounds(); });

    imgui_ext::FocusGroupInputColor(
        "Color", selection->GetColor(),
        [&](moth_ui::Color const& value) { m_editorLayer.BeginEditColor(); selection->SetColor(value); },
        [&]() { m_editorLayer.EndEditColor(); });

    auto blendMode = selection->GetBlendMode();
    if (imgui_ext::Inspect("Blend Mode", blendMode)) {
        selection->SetBlendMode(blendMode);
        entity->SetBlendMode(blendMode);
    }
}

void PropertiesEditor::DrawGroupProperties() {
}

void PropertiesEditor::DrawImageProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const imageNode = std::static_pointer_cast<moth_ui::NodeImage>(selection);
    auto const imageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(selection->GetLayoutEntity());
    // do we want to allow the animation of the source rect? would be nice
    imgui_ext::FocusGroupInputRect(
        "Source Rect", imageEntity->m_sourceRect,
        [&](moth_ui::IntRect const& value) {
            /*m_editorLayer.BeginEditBounds()*/;
            imageEntity->m_sourceRect = value;
            selection->ReloadEntity();
        },
        [&]() {
            /*m_editorLayer.EndEditBounds();*/
        });

    auto const oldImageScaleType = imageEntity->m_imageScaleType;
    imgui_ext::Inspect("Image Scale Type", imageEntity->m_imageScaleType);
    if (imageEntity->m_imageScaleType != oldImageScaleType) {
        selection->ReloadEntity();
    }

    auto const oldImageScale= imageEntity->m_imageScale;
    imgui_ext::Inspect("Image Scale", imageEntity->m_imageScale);
    if (imageEntity->m_imageScale!= oldImageScale) {
        selection->ReloadEntity();
    }

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
            targetImageEntity->m_texturePath = s_fileBrowser.GetSelected().string();
            s_loadingNodeImage->ReloadEntity();
            s_fileBrowser.ClearSelected();
            s_loadingNodeImage = nullptr;
        }
    }
}

void PropertiesEditor::DrawTextProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const textNode = std::static_pointer_cast<moth_ui::NodeText>(selection);
    auto const textEntity = std::static_pointer_cast<moth_ui::LayoutEntityText>(selection->GetLayoutEntity());
    bool changed = false;

    auto const oldSize = textEntity->m_fontSize;
    imgui_ext::Inspect("Font Size", textEntity->m_fontSize);
    if (oldSize != textEntity->m_fontSize) {
        changed = true;
    }

    auto const fontNames = moth_ui::Context::GetCurrentContext().GetFontFactory().GetFontNameList();
    if (ImGui::BeginCombo("Font", textEntity->m_fontName.c_str())) {
        for (auto fontName : fontNames) {
            bool selected = fontName == textEntity->m_fontName;
            if (ImGui::Selectable(fontName.c_str(), selected)) {
                textEntity->m_fontName = fontName;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    changed |= imgui_ext::Inspect("Horiz Alignment", textEntity->m_horizontalAlignment);
    changed |= imgui_ext::Inspect("Vert Alignment", textEntity->m_verticalAlignment);

    imgui_ext::Inspect("Text", textEntity->m_text);

    if (changed) {
        textNode->ReloadEntity();
    }
}
