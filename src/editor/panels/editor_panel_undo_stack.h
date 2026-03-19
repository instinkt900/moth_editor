#pragma once

#include "editor_panel.h"

class EditorPanelUndoStack : public EditorPanel {
public:
    EditorPanelUndoStack(EditorLayer& editorLayer, bool visible);
    ~EditorPanelUndoStack() override = default;

private:
    void DrawContents() override;
};
