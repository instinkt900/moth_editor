#pragma once

#include "editor_panel.h"

class EditorPanelCanvas;

class EditorPanelCanvasProperties : public EditorPanel {
public:
    EditorPanelCanvasProperties(EditorLayer& editorLayer, bool visible, EditorPanelCanvas& canvasPanel);
    virtual ~EditorPanelCanvasProperties() = default;

private:
    EditorPanelCanvas& m_canvasPanel;
    void DrawContents() override;
};
