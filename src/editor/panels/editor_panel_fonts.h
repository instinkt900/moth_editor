#pragma once

#include "moth/ui/utils/vector.h"

#include <filesystem>
#include <memory>

namespace moth::gfx {
    class ITarget;
}

class EditorLayer;

class EditorPanelFonts {
public:
    explicit EditorPanelFonts(EditorLayer& editorLayer);
    ~EditorPanelFonts();

    void Open() { m_open = true; }
    void Draw();

private:
    EditorLayer& m_editorLayer;
    bool m_open = false;

    int m_selectedIndex = -1;
    std::filesystem::path m_pendingFontPath;

    std::unique_ptr<moth::gfx::ITarget> m_previewTarget;
    moth::ui::IntVec2 m_previewTargetSize{ 0, 0 };
};
