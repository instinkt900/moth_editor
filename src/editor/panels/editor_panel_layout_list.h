#pragma once

#include "editor_panel.h"
#include "../confirm_prompt.h"
#include "../content_list.h"

class EditorPanelLayoutList : public EditorPanel {
public:
    EditorPanelLayoutList(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelLayoutList() = default;

    void Refresh() override;

private:
    ContentList m_contentList;
    ConfirmPrompt m_deleteConfirm;

    void DrawContents() override;
};
