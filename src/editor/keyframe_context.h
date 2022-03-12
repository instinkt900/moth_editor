#pragma once

#include "moth_ui/animation_track.h"
#include "moth_ui/utils/imgui_ext_focus.h"

struct KeyframeContext {
    std::shared_ptr<moth_ui::LayoutEntity> entity;
    moth_ui::AnimationTrack::Target target = moth_ui::AnimationTrack::Target::Unknown;
    int frameNo = -1;
    moth_ui::Keyframe* current = nullptr;
    std::optional<moth_ui::Keyframe> tempEditable;
    imgui_ext::FocusGroupContext focusContext;
};
