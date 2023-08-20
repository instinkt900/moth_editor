#pragma once

#include "editor_panel.h"
#include "../confirm_prompt.h"
#include "../content_list.h"

class EditorPanelFonts : public EditorPanel {
public:
    EditorPanelFonts(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelFonts() = default;

private:
    void DrawContents() override;

    int m_selectedIndex = -1;
    std::filesystem::path m_pendingFontPath;
};
