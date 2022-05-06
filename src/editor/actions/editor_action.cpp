#include "common.h"
#include "editor_action.h"
#include "moth_ui/node.h"
#include "../editor_layer.h"

//std::unique_ptr<IEditorAction> MakeVisibilityAction(std::shared_ptr<moth_ui::Node> node, bool visible) {
//    bool const oldVisible = node->IsVisible();
//    return std::make_unique<BasicAction>([node, visible]() { node->SetVisible(visible); }, [node, oldVisible]() { node->SetVisible(oldVisible); });
//}

std::unique_ptr<IEditorAction> MakeLockAction(std::shared_ptr<moth_ui::Node> node, bool locked, EditorLayer& editorLayer) {
    EditorLayer* layer = &editorLayer;
    return std::make_unique<BasicAction>([node, layer, locked]() {
                                            if (locked) {
                                                layer->LockNode(node);
                                            } else {
                                                layer->UnlockNode(node);
                                            } },
                                         [node, layer, locked]() {
                                             if (locked) {
                                                 layer->UnlockNode(node);
                                             } else {
                                                 layer->LockNode(node);
                                             }
                                         });
}

std::unique_ptr<IEditorAction> MakeShowBoundsAction(std::shared_ptr<moth_ui::Node> node, bool visible) {
    return std::make_unique<BasicAction>([node, visible]() { node->SetShowRect(visible); }, [node, visible]() { node->SetShowRect(!visible); });
}
