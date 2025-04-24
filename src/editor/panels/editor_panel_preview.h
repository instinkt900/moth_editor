#pragma once

#include "editor_panel.h"
#include "canyon/graphics/itarget.h"
#include <canyon/graphics/moth_ui/moth_image.h>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>

class EditorPanelPreview : public EditorPanel {
public:
    EditorPanelPreview(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelPreview() = default;

    bool OnEvent(moth_ui::Event const& event) override;
    void Update(uint32_t ticks) override;

private:
    void DrawContents() override;

    bool m_wasVisible = false;
    std::unique_ptr<moth_ui::Node> m_root;
    std::vector<std::string> m_clipNames;
    std::string m_selectedClip;

    std::shared_ptr<canyon::graphics::ITarget> m_renderSurface;
    moth_ui::IntVec2 m_currentSurfaceSize;

    void SetLayout(std::shared_ptr<moth_ui::Layout> layout);
    void UpdateRenderSurface(moth_ui::IntVec2 surfaceSize);
};
