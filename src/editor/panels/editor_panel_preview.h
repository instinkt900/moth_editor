#pragma once

#include "editor_panel.h"
#include "moth_graphics/graphics/itarget.h"
#include <moth_graphics/graphics/moth_ui/moth_image.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

class EditorPanelPreview : public EditorPanel {
public:
    EditorPanelPreview(EditorLayer& editorLayer, bool visible);
    ~EditorPanelPreview() override = default;

    bool OnEvent(moth_ui::Event const& event) override;
    void Update(uint32_t ticks) override;

private:
    void DrawContents() override;

    bool m_wasVisible = false;
    std::shared_ptr<moth_ui::Node> m_root;
    std::vector<std::string> m_clipNames;
    std::string m_selectedClip;

    std::shared_ptr<moth_graphics::graphics::ITarget> m_renderSurface;
    moth_ui::IntVec2 m_currentSurfaceSize;

    void SetLayout(std::shared_ptr<moth_ui::Layout> layout);
    void UpdateRenderSurface(moth_ui::IntVec2 surfaceSize);
};
