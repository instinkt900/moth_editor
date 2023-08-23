#pragma once

#include "moth_ui/layers/layer_stack.h"

class EditorLayerStack : public moth_ui::LayerStack {
public:
    EditorLayerStack(int renderWidth, int renderHeight, int windowWidth, int windowHeight);
    virtual ~EditorLayerStack() = default;

    void DebugDraw() override;

protected:
    void SetLayerLogicalSize(moth_ui::IntVec2 const& size) override;
};
