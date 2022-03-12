#pragma once

#include "moth_ui/animation_track.h"

struct KeyframeContext;
class MoveKeyframeAction;
class EditorLayer;

class KeyframeWidget {
public:
    KeyframeWidget(EditorLayer& editorLayer, std::vector<KeyframeContext>& selectedKeyframes);
    ~KeyframeWidget();

    void SetOpen(bool open) { m_open = open; }

    void Draw();

private:
    bool m_open = false;
    EditorLayer& m_editorLayer;
    std::vector<KeyframeContext>& m_selectedKeyframes;

    void DrawKeyframe(KeyframeContext& context);

    std::optional<moth_ui::Keyframe> m_originalKeyframe;

    void BeginEdit(KeyframeContext& context);
    void EndEdit(KeyframeContext& context);
};
