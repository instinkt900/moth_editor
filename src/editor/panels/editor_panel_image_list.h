#pragma once

#include "editor_panel.h"

class EditorPanelImageList : public EditorPanel {
public:
    EditorPanelImageList(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelImageList() = default;

    void Refresh() override;

private:
    struct ImageInfo {
        std::string m_path;
        std::string m_name;
    };

    int m_selectedIndex = -1;
    std::vector<ImageInfo> m_imageList;

    void DrawContents() override;
};
