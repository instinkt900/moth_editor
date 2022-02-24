#pragma once

namespace ui {
    struct KeyframeContext;
    class MoveKeyframeAction;
    class EditorLayer;
    class Keyframe;

    class KeyframeWidget {
    public:
        KeyframeWidget(EditorLayer& editorLayer, std::vector<KeyframeContext>& selectedKeyframes);
        ~KeyframeWidget();

        void Draw();

    private:
        EditorLayer& m_editorLayer;
        std::vector<KeyframeContext>& m_selectedKeyframes;
        MoveKeyframeAction* m_lastMoveAction = nullptr;

        struct MoveContext {
            KeyframeContext* context;
            int originalFrameNo;
            Keyframe* existingFrame = nullptr;
        };
        std::unique_ptr<MoveContext> m_currentMoveContext;

        void DrawKeyframe(KeyframeContext& context);
        void OnFrameChanged(KeyframeContext& context, int destFrameNo);
        void CheckLostFocus(KeyframeContext& context);
        void OnMoveComplete();
    };
}
