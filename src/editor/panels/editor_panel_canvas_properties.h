#pragma once

#include "editor_panel.h"

class EditorPanelCanvasProperties : public EditorPanel {
public:
    EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelCanvasProperties() = default;

private:
    void DrawContents() override;
};
