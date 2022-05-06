#pragma once

#include "moth_ui/ui_fwd.h"
#include "moth_ui/utils/vector.h"

namespace imgui_ext {
    bool InputString(char const* label, std::string* str);
    bool InputKeyframeValue(char const* label, moth_ui::KeyframeValue* value);

    void InputIntVec2(char const* label, moth_ui::IntVec2* vec);
    void InputFloatVec2(char const* label, moth_ui::FloatVec2* vec);
}
