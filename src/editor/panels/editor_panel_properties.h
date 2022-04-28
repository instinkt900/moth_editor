#pragma once

#include "editor_panel.h"
#include "moth_ui/utils/imgui_ext_focus.h"

class EditorPanelProperties : public EditorPanel {
public:
    EditorPanelProperties(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelProperties() = default;

private:
    void DrawContents() override;

    imgui_ext::FocusGroupContext m_focusContext;

    void DrawEntityProperties(std::shared_ptr<moth_ui::Node> node);
    void DrawRectProperties(std::shared_ptr<moth_ui::Node> node);
    void DrawImageProperties(std::shared_ptr<moth_ui::Node> node);
    void DrawTextProperties(std::shared_ptr<moth_ui::Node> node);
};
