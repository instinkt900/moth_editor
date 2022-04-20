#include "common.h"
#include "editor_panel.h"

EditorPanel::EditorPanel(EditorLayer& editorLayer, std::string const& title, bool visible, bool exposed)
    : m_editorLayer(editorLayer)
    , m_title(title)
    , m_visible(visible)
    , m_exposed(exposed) {
}

void EditorPanel::Draw() {
    if (!m_wasVisible && m_visible) {
        OnOpen();
    } else if (m_wasVisible && !m_visible) {
        OnClose();
    }

    m_wasVisible = m_visible;

    if (m_visible) {
        if (ImGui::Begin(m_title.c_str(), &m_visible)) {
            DrawContents();
        }
        ImGui::End();
    }
}
