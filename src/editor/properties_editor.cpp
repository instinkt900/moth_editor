#include "common.h"
#include "properties_editor.h"
#include "editor_layer.h"
#include "moth_ui/layout/layout_entity_image.h"

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
}

void PropertiesEditor::DrawGroupProperties() {
}

void PropertiesEditor::DrawImageProperties() {
}
