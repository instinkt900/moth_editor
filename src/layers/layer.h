#pragma once

#include "uilib/event_listener.h"

class LayerStack;

class Layer : public EventListener {
public:
    Layer();
    virtual ~Layer();

    bool OnEvent(Event const& event) override;

    virtual void Update(uint32_t ticks);
    virtual void Draw(SDL_Renderer& renderer);
    virtual void DebugDraw();

    virtual void OnAddedToStack(LayerStack* layerStack);
    virtual void OnRemovedFromStack();

    int GetWidth() const;
    int GetHeight() const;

    virtual bool UseRenderSize() const { return true; }

protected:
    LayerStack* m_layerStack = nullptr;
};
