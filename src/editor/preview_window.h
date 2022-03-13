#pragma once

#include "moth_ui/event_listener.h"

class PreviewWindow : public moth_ui::EventListener {
public:
    PreviewWindow();
    ~PreviewWindow() = default;

    bool OnEvent(moth_ui::Event const& event);
    void Update(uint32_t ticks);
    void Draw();

    void Refresh(std::shared_ptr<moth_ui::LayoutEntityGroup> layout);

private:
    std::unique_ptr<moth_ui::Node> m_root;
    std::vector<std::string> m_clipNames;
    std::string m_selectedClip;

    TextureRef m_renderSurface;
    moth_ui::IntVec2 m_currentSurfaceSize;

    void UpdateRenderSurface(moth_ui::IntVec2 surfaceSize);
};
