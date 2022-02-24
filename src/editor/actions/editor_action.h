#pragma once

namespace ui {
    class IEditorAction {
    public:
        virtual ~IEditorAction() = default;

        virtual void Do() = 0;
        virtual void Undo() = 0;

        virtual void OnImGui() = 0;
    };
}
