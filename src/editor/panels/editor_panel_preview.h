#pragma once

#include "editor_panel.h"
#include "moth/graphics/graphics/itarget.h"
#include <moth/bridge/moth_image.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

class EditorPanelPreview : public EditorPanel {
public:
    EditorPanelPreview(EditorLayer& editorLayer, bool visible);
    ~EditorPanelPreview() override = default;

    bool OnEvent(moth::ui::Event const& event) override;
    void Update(uint32_t ticks) override;

private:
    void DrawContents() override;

    bool m_wasVisible = false;
    std::shared_ptr<moth::ui::Node> m_root;
    std::vector<std::string> m_clipNames;
    std::string m_selectedClip;

    std::shared_ptr<moth::gfx::ITarget> m_renderSurface;
    moth::ui::IntVec2 m_currentSurfaceSize;

    void SetLayout(std::shared_ptr<moth::ui::Layout> layout);
    void UpdateRenderSurface(moth::ui::IntVec2 surfaceSize);
};
