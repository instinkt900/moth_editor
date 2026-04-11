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
}
