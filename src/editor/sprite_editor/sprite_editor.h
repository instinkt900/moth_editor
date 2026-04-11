#pragma once

#include "editor/actions/editor_action.h"
#include "moth_graphics/graphics/spritesheet.h"

#include <filesystem>
#include <memory>
#include <optional>
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
    void DrawFramesPane();
    void DrawClipsPane();

    // Undo/redo stack (independent from the main editor's stack)
    void AddSpriteAction(std::unique_ptr<IEditorAction> action);
    void UndoSpriteAction();
    void RedoSpriteAction();
    void ClearSpriteActions();

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

    // Undo stack
    std::vector<std::unique_ptr<IEditorAction>> m_undoStack;
    int m_undoIndex = -1;

    // Deferred snapshot for InputInt/InputText commits (captured on activate, pushed on deactivate)
    using FrameVec = std::vector<moth_graphics::graphics::SpriteSheet::FrameEntry>;
    using ClipVec  = std::vector<moth_graphics::graphics::SpriteSheet::ClipEntry>;
    std::optional<FrameVec> m_pendingFrameSnapshot;
    std::optional<ClipVec>  m_pendingClipSnapshot;

    // Pivot drag state (click-drag in frame mini-preview)
    bool m_pivotDragging = false;
    std::optional<FrameVec> m_pivotDragSnapshot;
};
