#pragma once

#include "moth_ui/events/event.h"

namespace backend::vulkan {
    std::unique_ptr<moth_ui::Event> FromGLFW(int key, int scancode, int action, int mods);
}
