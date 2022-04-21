#pragma once

#include "editor_panel.h"
#include "../content_list.h"

class EditorPanelImageList : public EditorPanel {
public:
    EditorPanelImageList(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelImageList() = default;

    void Refresh() override;

private:
    ContentList m_contentList;

    void DrawContents() override;
};
