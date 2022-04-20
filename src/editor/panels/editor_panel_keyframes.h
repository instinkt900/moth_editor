#pragma once

#include "editor_panel.h"
#include "moth_ui/animation_track.h"

struct KeyframeContext;
class EditorPanelAnimation;

class EditorPanelKeyframes : public EditorPanel {
public:
    EditorPanelKeyframes(EditorLayer& editorLayer, EditorPanelAnimation& animationPanel);
    virtual ~EditorPanelKeyframes() = default;

    void Update(uint32_t ticks) override;

private:
    void DrawContents() override;

    void OnClose() override;

    std::vector<KeyframeContext>& m_selectedKeyframes;
    std::optional<moth_ui::Keyframe> m_originalKeyframe;

    void DrawKeyframeValues(KeyframeContext& context);
    void BeginEdit(KeyframeContext& context);
    void EndEdit(KeyframeContext& context);
};
