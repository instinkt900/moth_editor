#include "common.h"
#include "editor_action.h"
#include "moth_ui/node.h"

std::unique_ptr<IEditorAction> MakeVisibilityAction(std::shared_ptr<moth_ui::Node> node, bool visible) {
    return std::make_unique<BasicAction>([node, visible]() { node->SetVisible(visible); }, [node, visible]() { node->SetVisible(!visible); });
}

std::unique_ptr<IEditorAction> MakeShowBoundsAction(std::shared_ptr<moth_ui::Node> node, bool visible) {
    return std::make_unique<BasicAction>([node, visible]() { node->SetShowRect(visible); }, [node, visible]() { node->SetShowRect(!visible); });
}
