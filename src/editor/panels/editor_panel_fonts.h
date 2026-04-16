#pragma once

#include <filesystem>

class EditorLayer;

class EditorPanelFonts {
public:
    explicit EditorPanelFonts(EditorLayer& editorLayer);
    ~EditorPanelFonts() = default;

    void Open() { m_open = true; }
    void Draw();

private:
    EditorLayer& m_editorLayer;
    bool m_open = false;

    int m_selectedIndex = -1;
    std::filesystem::path m_pendingFontPath;
};
