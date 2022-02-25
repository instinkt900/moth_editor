#pragma once

#include "moth_ui/event_listener.h"

class Layer;

class LayerStack : public moth_ui::EventListener {
public:
    LayerStack(int renderWidth, int renderHeight, int windowWidth, int windowHeight);
    ~LayerStack();

    void PushLayer(std::unique_ptr<Layer>&& layer);
    std::unique_ptr<Layer> PopLayer();
    void RemoveLayer(Layer* layer);

    bool OnEvent(moth_ui::Event const& event) override;
    void Update(uint32_t ticks);
    void Draw(SDL_Renderer& renderer);
    void DebugDraw();

    int GetRenderWidth() const { return m_renderWidth; }
    int GetRenderHeight() const { return m_renderHeight; }
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }

private:
    std::vector<std::unique_ptr<Layer>> m_layers;

    int m_renderWidth = 0;
    int m_renderHeight = 0;

    int m_windowWidth = 0;
    int m_windowHeight = 0;
};