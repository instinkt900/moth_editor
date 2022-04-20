#pragma once

#include "editor_panel.h"

class EditorPanelElements : public EditorPanel {
public:
    EditorPanelElements(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelElements() = default;

private:
    void DrawContents() override;
};
