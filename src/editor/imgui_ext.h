#pragma once

#include "moth/ui/moth_ui_fwd.h"
#include "moth/ui/utils/vector.h"
#include "moth/graphics/graphics/image.h"

#include <string>

namespace moth::gfx::platform {
    class ImGuiContext;
}

namespace imgui_ext {
    // The toolkit draws textures through the platform ImGui context rather than from
    // Image itself, so the context is registered once at startup and the Image
    // helpers below draw through it.
    void SetImGuiContext(moth::gfx::platform::ImGuiContext* context);

    bool InputString(char const* label, std::string* str);
    bool InputKeyframeValue(char const* label, moth::ui::KeyframeValue* value);

    void InputIntVec2(char const* label, moth::ui::IntVec2* vec);
    void InputFloatVec2(char const* label, moth::ui::FloatVec2* vec);

    void Image(moth::gfx::Image const& image, int width, int height);
    void Image(moth::gfx::Image const& image, int width, int height,
               moth::ui::FloatVec2 const& uv0, moth::ui::FloatVec2 const& uv1);
    void Image(moth::ui::IImage const* image, int width, int height);
}
