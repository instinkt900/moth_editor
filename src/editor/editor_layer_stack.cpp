#include "common.h"
#include "editor_layer_stack.h"
#include "editor_application.h"
#include "moth_ui/layers/layer.h"

EditorLayerStack::EditorLayerStack(int renderWidth, int renderHeight, int windowWidth, int windowHeight)
    : LayerStack(renderWidth, renderHeight, windowWidth, windowHeight) {
}

void EditorLayerStack::SetLayerLogicalSize(moth_ui::IntVec2 const& size) {
    g_App->GetGraphics().SetLogicalSize(size);
}

void EditorLayerStack::DebugDraw() {
    if (ImGui::Begin("Layer Stack")) {
        for (auto&& layer : m_layers) {
            layer->DebugDraw();
        }
    }
    ImGui::End();
}
