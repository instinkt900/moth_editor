#pragma once

#include "uilib/animation_track.h"

namespace ui {
    class LayoutEntity;

    struct KeyframeContext {
        std::shared_ptr<LayoutEntity> entity;
        AnimationTrack::Target target = AnimationTrack::Target::Unknown;
        int frameNo = -1;
        Keyframe* current = nullptr;
    };
}
