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

    void DrawEntityProperties();
    void DrawRectProperties();
    void DrawImageProperties();
    void DrawTextProperties();
    void Draw9SliceProperties();
};
