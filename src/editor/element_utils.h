#pragma once

#include "editor_layer.h"
#include "actions/add_action.h"

template <typename T, typename... Args>
void AddEntityWithBounds(EditorLayer& editorLayer, moth_ui::LayoutRect const& bounds, Args&&... args) {
    auto newLayoutEntity = std::make_shared<T>(bounds, std::forward<Args>(args)...);
    auto instance = newLayoutEntity->Instantiate();
    auto addAction = std::make_unique<AddAction>(std::move(instance), editorLayer.GetRoot());
    editorLayer.PerformEditAction(std::move(addAction));
    editorLayer.GetRoot()->RecalculateBounds();
}

template <typename T, typename... Args>
void AddEntity(EditorLayer& editorLayer, Args&&... args) {
    moth_ui::LayoutRect bounds;
    bounds.anchor.topLeft = { 0.5f, 0.5f };
    bounds.anchor.bottomRight = { 0.5f, 0.5f };
    bounds.offset.topLeft = { -50, -50 };
    bounds.offset.bottomRight = { 50, 50 };
    AddEntityWithBounds<T, Args...>(editorLayer, bounds, std::forward<Args>(args)...);
}

