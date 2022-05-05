#pragma once

#include "editor_panel.h"
#include "../confirm_prompt.h"
#include "../content_list.h"

class EditorPanelAssetList : public EditorPanel {
public:
    EditorPanelAssetList(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelAssetList() = default;

    void OnLayoutLoaded() override { Refresh(); }
    void Refresh() override;

private:
    ContentList m_contentList;

    void DrawContents() override;
};
