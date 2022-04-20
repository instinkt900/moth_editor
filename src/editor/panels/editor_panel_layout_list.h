#pragma once

#include "editor_panel.h"
#include "../confirm_prompt.h"

class EditorPanelLayoutList : public EditorPanel {
public:
    EditorPanelLayoutList(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelLayoutList() = default;

    void Refresh() override;

private:
    struct LayoutInfo {
        std::string m_path;
        std::string m_name;
    };

    int m_selectedIndex = -1;
    std::vector<LayoutInfo> m_layoutList;
    std::filesystem::path m_currentPath;
    ConfirmPrompt m_deleteConfirm;

    void DrawContents() override;
};
