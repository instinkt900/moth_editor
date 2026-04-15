#include "common.h"
#include "sprite_editor.h"

void SpriteEditor::AddSpriteAction(std::unique_ptr<IEditorAction> action) {
    while (static_cast<int>(m_undoStack.size()) - 1 > m_undoIndex) {
        m_undoStack.pop_back();
    }
    m_undoStack.push_back(std::move(action));
    ++m_undoIndex;
}

void SpriteEditor::UndoSpriteAction() {
    if (m_undoIndex >= 0) {
        m_undoStack[m_undoIndex]->Undo();
        --m_undoIndex;
    }
}

void SpriteEditor::RedoSpriteAction() {
    if (m_undoIndex < static_cast<int>(m_undoStack.size()) - 1) {
        ++m_undoIndex;
        m_undoStack[m_undoIndex]->Do();
    }
}

void SpriteEditor::ClearSpriteActions() {
    m_undoStack.clear();
    m_undoIndex = -1;
    m_pendingFrameSnapshot.reset();
    m_pendingClipSnapshot.reset();
    m_pivotDragging = false;
    m_pivotDragSnapshot.reset();
    m_frameDrag.reset();
}

void SpriteEditor::PushFrameAction(FrameVec before, int selBefore, int selAfter) {
    AddSpriteAction(std::make_unique<BasicAction>(
        [this, after = m_frames, selAfter]()           { m_frames = after;  m_selectedFrame = selAfter; },
        [this, b = std::move(before), selBefore]()     { m_frames = b;      m_selectedFrame = selBefore; }
    ));
}

void SpriteEditor::PushClipAction(ClipVec before, int selBefore, int selAfter) {
    AddSpriteAction(std::make_unique<BasicAction>(
        [this, after = m_clips, selAfter]()            { m_clips = after;   m_selectedClip = selAfter; },
        [this, b = std::move(before), selBefore]()     { m_clips = b;       m_selectedClip = selBefore; }
    ));
}

void SpriteEditor::PushFrameClipAction(FrameVec beforeF, ClipVec beforeC, int selBefore, int selAfter) {
    AddSpriteAction(std::make_unique<BasicAction>(
        [this, af = m_frames, ac = m_clips, selAfter]() {
            m_frames = af; m_clips = ac; m_selectedFrame = selAfter;
        },
        [this, bf = std::move(beforeF), bc = std::move(beforeC), selBefore]() {
            m_frames = bf; m_clips = bc; m_selectedFrame = selBefore;
        }
    ));
}
