#pragma once

#include "moth_graphics/graphics/spritesheet.h"

#include <filesystem>
#include <memory>
#include <vector>

class EditorLayer;

class SpriteEditor {
public:
    explicit SpriteEditor(EditorLayer& editorLayer);
    ~SpriteEditor() = default;

    void Open() { m_open = true; }
    void Draw();

private:
    void LoadSpriteSheet(std::filesystem::path const& path);
    void DrawPreview();
    void DrawDataEditor();

    EditorLayer& m_editorLayer;
    bool m_open = false;
    char m_pathBuffer[1024] = {};
    std::shared_ptr<moth_graphics::graphics::SpriteSheet> m_spriteSheet;
    std::vector<moth_graphics::graphics::SpriteSheet::FrameEntry> m_frames;
    int m_selectedFrame = -1;
    float m_zoom = 1.0f; // -1 = auto-fit on next draw
};
