#include "common.h"
#include "editor_panel_undo_stack.h"
#include "../editor_layer.h"
#include "../actions/editor_action.h"

EditorPanelUndoStack::EditorPanelUndoStack(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Undo Stack", visible, true) {
}

void EditorPanelUndoStack::DrawContents() {
    auto const& editActions = m_editorLayer.GetEditActions();
    auto const editActionPos = m_editorLayer.GetEditActionPos();

    int i = 0;
    for (auto&& edit : editActions) {
        ImGui::PushID(edit.get());
        if (i == editActionPos) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        }
        edit->OnImGui();
        if (i == editActionPos) {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
        ++i;
    }
}
