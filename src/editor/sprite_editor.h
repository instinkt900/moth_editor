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
    void ImportSheet(std::filesystem::path const& imagePath);
    void SaveSpriteSheet();
    void DrawPreview();
    void DrawDataEditor();

    EditorLayer& m_editorLayer;
    bool m_open = false;
    char m_pathBuffer[1024] = {};
    char m_imagePathBuffer[1024] = {};
    std::shared_ptr<moth_graphics::graphics::SpriteSheet> m_spriteSheet;
    std::vector<moth_graphics::graphics::SpriteSheet::FrameEntry> m_frames;
    std::vector<moth_graphics::graphics::SpriteSheet::ClipEntry> m_clips;
    int m_selectedFrame = -1;
    float m_zoom = 1.0f; // -1 = auto-fit on next draw
    char m_newClipNameBuffer[256] = {};
    int m_selectedClip = -1;
    bool m_clipPlaying = false;
    int m_clipCurrentStep = 0;
    float m_clipElapsedMs = 0.0f;
};
