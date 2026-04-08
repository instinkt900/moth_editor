#pragma once

#include "editor_panel.h"
#include "moth_graphics/graphics/spritesheet.h"

#include <filesystem>
#include <memory>

class EditorPanelSpriteEditor : public EditorPanel {
public:
    EditorPanelSpriteEditor(EditorLayer& editorLayer, bool visible);
    ~EditorPanelSpriteEditor() override = default;

private:
    void DrawContents() override;
    void LoadSpriteSheet(std::filesystem::path const& path);

    char m_pathBuffer[1024] = {};
    std::filesystem::path m_loadedPath;
    std::shared_ptr<moth_graphics::graphics::SpriteSheet> m_spriteSheet;
};
