#pragma once

#include "editor_panel.h"
#include "../layout_project.h"

class EditorPanelProjectProperties : public EditorPanel {
public:
    EditorPanelProjectProperties(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelProjectProperties() = default;

private:
    void OnOpen() override;
    void OnClose() override;
    void DrawContents() override;

    LayoutProject m_tempProject;
};
