#pragma once

#include "editor_panel.h"

class BoundsWidget;

class EditorPanelCanvas : public EditorPanel {
public:
    EditorPanelCanvas(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelCanvas() = default;

private:
    bool BeginPanel() override;
    void DrawContents() override;
    void EndPanel() override;

    TextureRef m_displayTexture;
    moth_ui::IntVec2 m_currentDisplaySize;
    moth_ui::FloatVec2 m_initialCanvasOffset;
    bool m_dragging = false;
    bool m_wasLeftDown = false;
    moth_ui::FloatVec2 m_lastMousePos;

    std::unique_ptr<BoundsWidget> m_boundsWidget;

    void UpdateDisplayTexture(SDL_Renderer& renderer, moth_ui::IntVec2 const displaySize);
};
