#pragma once

#include "moth_ui/utils/color.h"
#include "moth_ui/animation_track.h"
#include "moth_ui/utils/rect.h"

namespace imgui_ext {
    struct FocusGroupContext {
        std::string currentFocusLabel;
    };
    
    void FocusGroupBegin(FocusGroupContext* context);
    void FocusGroupInputText(char const* label, std::string const& target, std::function<void(std::string const&)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputInt(char const* label, int value, std::function<void(int)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputFloat(char const* label, float value, std::function<void(float)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputRect(char const* label, moth_ui::IntRect value, std::function<void(moth_ui::IntRect const&)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputLayoutRect(char const* label, moth_ui::LayoutRect value, std::function<void(moth_ui::LayoutRect const&)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputColor(char const* label, moth_ui::Color value, std::function<void(moth_ui::Color const&)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputKeyframeValue(char const* label, moth_ui::KeyframeValue value, std::function<void(moth_ui::KeyframeValue const&)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupInputInterpType(char const* label, moth_ui::InterpType value, std::function<void(moth_ui::InterpType const&)> const& onChanged, std::function<void()> const& onLostFocus);
    void FocusGroupEnd();
}
