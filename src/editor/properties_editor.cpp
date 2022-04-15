#include "common.h"
#include "properties_editor.h"
#include "editor_layer.h"
#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/utils/imgui_ext_inspect.h"
#include "moth_ui/node_rect.h"
#include "moth_ui/node_image.h"
#include "moth_ui/node_text.h"
#include "utils.h"
#include "moth_ui/context.h"
#include "properties_elements.h"
#include "actions/change_id_action.h"
#include "actions/change_text_action.h"
#include "actions/set_visible_action.h"
#include "actions/show_bounds_action.h"
#include "actions/set_blend_mode_action.h"
#include "actions/set_image_scale_type_action.h"
#include "actions/set_image_scale_action.h"
#include "actions/change_image_path_action.h"
#include "actions/change_font_size_action.h"
#include "actions/change_font_action.h"
#include "actions/change_h_alignment_action.h"
#include "actions/change_v_alignment_action.h"

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

void PropertiesEditor::DrawEntityProperties() {
    int const frame = m_editorLayer.GetSelectedFrame();
    auto selection = m_editorLayer.GetSelection();
    auto entity = selection->GetLayoutEntity();

    PropertiesInput(
        "ID", entity->m_id.c_str(),
        [&](char const* changedValue) {
            selection->SetId(changedValue);
        },
        [=](char const* oldValue, char const* newValue) {
            auto changeIdAction = std::make_unique<ChangeIdAction>(selection, oldValue, newValue);
            changeIdAction->Do();
            m_editorLayer.AddEditAction(std::move(changeIdAction));
        });

    PropertiesInput("Visible", selection->IsVisible(), [&](bool value) {
        auto setVisibleAction = std::make_unique<SetVisibleAction>(selection, value);
        setVisibleAction->Do();
        m_editorLayer.AddEditAction(std::move(setVisibleAction));
    });

    PropertiesInput("Show Bounds", selection->GetShowRect(), [&](bool value) {
        auto showBoundsAction = std::make_unique<ShowBoundsAction>(selection, value);
        showBoundsAction->Do();
        m_editorLayer.AddEditAction(std::move(showBoundsAction));
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
            auto setBlendModeAction = std::make_unique<SetBlendModeAction>(selection, oldValue, newValue);
            setBlendModeAction->Do();
            m_editorLayer.AddEditAction(std::move(setBlendModeAction));
        });
}

void PropertiesEditor::DrawGroupProperties() {
}

void PropertiesEditor::DrawRectProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const rectNode = std::static_pointer_cast<moth_ui::NodeRect>(selection);
    auto const rectEntity = std::static_pointer_cast<moth_ui::LayoutEntityRect>(selection->GetLayoutEntity());

    PropertiesInput(
        "Filled", rectEntity->m_filled,
        [&](auto changedValue) {
            rectEntity->m_filled = changedValue;
            selection->ReloadEntity();
        });
}

void PropertiesEditor::DrawImageProperties() {
    auto const selection = m_editorLayer.GetSelection();
    auto const imageNode = std::static_pointer_cast<moth_ui::NodeImage>(selection);
    auto const imageEntity = std::static_pointer_cast<moth_ui::LayoutEntityImage>(selection->GetLayoutEntity());
    // do we want to allow the animation of the source rect? would be nice
    PropertiesInput(
        "Source Rect", imageEntity->m_sourceRect,
        [&](moth_ui::IntRect changedValue) {
            imageEntity->m_sourceRect = changedValue;
            selection->ReloadEntity();
        },
        nullptr);

    PropertiesInput(
        "Image Scale Type", imageEntity->m_imageScaleType,
        [&](auto oldValue, auto newValue) {
            auto setImageScaleTypeAction = std::make_unique<SetImageScaleTypeAction>(selection, oldValue, newValue);
            setImageScaleTypeAction->Do();
            m_editorLayer.AddEditAction(std::move(setImageScaleTypeAction));
        });

    PropertiesInput(
        "Image Scale", imageEntity->m_imageScale,
        [&](float changedValue) {
            imageEntity->m_imageScale = changedValue;
            selection->ReloadEntity();
        },
        [=](float oldValue, float newValue) {
            auto setImageScaleAction = std::make_unique<SetImageScaleAction>(selection, oldValue, newValue);
            setImageScaleAction->Do();
            m_editorLayer.AddEditAction(std::move(setImageScaleAction));
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
            auto const oldPath = targetImageEntity->m_texturePath;
            auto const newPath = s_fileBrowser.GetSelected().string();
            auto changeImagePathAction = std::make_unique<ChangeImagePathAction>(selection, oldPath, newPath);
            changeImagePathAction->Do();
            m_editorLayer.AddEditAction(std::move(changeImagePathAction));
            s_fileBrowser.ClearSelected();
            s_loadingNodeImage = nullptr;
        }
    }
}

void PropertiesEditor::DrawTextProperties() {
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
            auto changeFontSizeAction = std::make_unique<ChangeFontSizeAction>(selection, oldValue, newValue);
            changeFontSizeAction->Do();
            m_editorLayer.AddEditAction(std::move(changeFontSizeAction));
        });

    auto const fontNames = moth_ui::Context::GetCurrentContext().GetFontFactory().GetFontNameList();
    PropertiesInput("Font", fontNames, textEntity->m_fontName,
                    [&](auto oldValue, auto newValue) {
                        auto changeFontAction = std::make_unique<ChangeFontAction>(selection, oldValue, newValue);
                        changeFontAction->Do();
                        m_editorLayer.AddEditAction(std::move(changeFontAction));
                    });

    PropertiesInput("H Alignment", textEntity->m_horizontalAlignment, [&](auto oldValue, auto newValue) {
        auto changeHAlignmentAction = std::make_unique<ChangeHAlignmentAction>(selection, oldValue, newValue);
        changeHAlignmentAction->Do();
        m_editorLayer.AddEditAction(std::move(changeHAlignmentAction));
    });

    PropertiesInput("V Alignment", textEntity->m_verticalAlignment, [&](auto oldValue, auto newValue) {
        auto changeVAlignmentAction = std::make_unique<ChangeVAlignmentAction>(selection, oldValue, newValue);
        changeVAlignmentAction->Do();
        m_editorLayer.AddEditAction(std::move(changeVAlignmentAction));
    });

    PropertiesInput(
        "Text", textEntity->m_text.c_str(), 8,
        [&](char const* changedValue) {
            textNode->SetText(changedValue);
        },
        [=](char const* oldValue, char const* newValue) {
            auto changeTextAction = std::make_unique<ChangeTextAction>(textNode, oldValue, newValue);
            changeTextAction->Do();
            m_editorLayer.AddEditAction(std::move(changeTextAction));
        });
}
