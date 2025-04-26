#pragma once

#include "moth_ui/layers/layer.h"
#include "moth_ui/events/event_animation.h"
#include "canyon/events/event_window.h"

#include <filesystem>

class ExampleLayer : public moth_ui::Layer {
public:
    ExampleLayer(moth_ui::Context& context, std::filesystem::path const& layoutPath);
    virtual ~ExampleLayer() = default;
    
    bool OnEvent(moth_ui::Event const& event) override;
    void Update(uint32_t ticks) override;
    void Draw() override;

    void OnAddedToStack(moth_ui::LayerStack* stack) override;
    void OnRemovedFromStack() override;

    void LoadLayout(std::filesystem::path const& path);
    std::shared_ptr<moth_ui::Group> GetUIRoot() { return m_root; }

protected:
    moth_ui::Context& m_context;
    std::shared_ptr<moth_ui::Group> m_root;
    moth_ui::IntVec2 m_lastDrawnSize;

    bool OnUIEvent(moth_ui::Event const& event);
    bool OnAnimationStopped(moth_ui::EventAnimationStopped const& event);
    bool OnRequestQuitEvent(canyon::EventRequestQuit const& event);
};
