#pragma once

#include "editor_action.h"

namespace ui {
    class CompositeAction : public IEditorAction {
    public:
        CompositeAction();
        virtual ~CompositeAction();

        auto& GetActions() { return m_actions; }

        void Do() override;
        void Undo() override;

        void OnImGui() override;

    protected:
        std::vector<std::unique_ptr<IEditorAction>> m_actions;
    };
}
