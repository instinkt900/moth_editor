#pragma once

#include "editor_panel.h"
#include "moth_ui/utils/imgui_ext_focus.h"

class EditorPanelProperties : public EditorPanel {
public:
    EditorPanelProperties(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelProperties() = default;

    void OnLayoutLoaded() override;

private:
    void DrawContents() override;

    moth_ui::Node* m_lastSelection = nullptr;

    imgui_ext::FocusGroupContext m_focusContext;

    void DrawNodeProperties(std::shared_ptr<moth_ui::Node> node, bool recurseChildren = true);
    void DrawCommonProperties(std::shared_ptr<moth_ui::Node> node);
    void DrawRectProperties(std::shared_ptr<moth_ui::NodeRect> node);
    void DrawImageProperties(std::shared_ptr<moth_ui::NodeImage> node);
    void DrawTextProperties(std::shared_ptr<moth_ui::NodeText> node);
    void DrawRefProperties(std::shared_ptr<moth_ui::Group> node, bool recurseChildren);
    void DrawLayoutProperties(std::shared_ptr<moth_ui::Group> node);
};
