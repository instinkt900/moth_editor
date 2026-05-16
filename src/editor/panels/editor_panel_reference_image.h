#pragma once

class EditorLayer;

class EditorPanelReferenceImage {
public:
    explicit EditorPanelReferenceImage(EditorLayer& editorLayer);
    ~EditorPanelReferenceImage() = default;

    void Open() { m_open = true; }
    void Draw();

private:
    EditorLayer& m_editorLayer;
    bool m_open = false;
};
