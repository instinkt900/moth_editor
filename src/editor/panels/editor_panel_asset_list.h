#pragma once

#include "editor_panel.h"
#include "../confirm_prompt.h"
#include "../content_list.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <memory>

namespace moth_graphics::graphics {
    class IImage;
}

class EditorPanelAssetList : public EditorPanel {
public:
    EditorPanelAssetList(EditorLayer& editorLayer, bool visible);
    ~EditorPanelAssetList() override;

    void OnLayoutLoaded() override { Refresh(); }
    void Refresh() override;

private:
    ContentList m_contentList;
    std::filesystem::file_time_type m_lastDirWriteTime;
    std::unordered_map<std::string, std::unique_ptr<moth_graphics::graphics::IImage>> m_imageCache;

    void DrawContents() override;
};
