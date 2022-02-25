#include "common.h"
#include "composite_action.h"

CompositeAction::CompositeAction() {
}

CompositeAction::~CompositeAction() {
}

void CompositeAction::Do() {
    for (auto&& action : m_actions) {
        action->Do();
    }
}

void CompositeAction::Undo() {
    // undo in reverse
    for (auto it = m_actions.rbegin(); it != std::rend(m_actions); ++it) {
        (*it)->Undo();
    }
}

void CompositeAction::OnImGui() {
    if (ImGui::CollapsingHeader("CompositeAction")) {
        for (auto&& action : m_actions) {
            action->OnImGui();
        }
    }
}
