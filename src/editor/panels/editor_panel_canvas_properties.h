#pragma once

#include "editor_panel.h"

class EditorPanelCanvasProperties : public EditorPanel {
public:
    EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible);
    ~EditorPanelCanvasProperties() override = default;

private:
    void DrawContents() override;
};
