#pragma once

bool PropertiesInput(char const* label, bool current, std::function<void(bool)> const& changeAction);
bool PropertiesInput(char const* label, int current, std::function<void(int)> const& changeAction, std::function<void(int, int)> const& commitAction);
bool PropertiesInput(char const* label, float current, std::function<void(float)> const& changeAction, std::function<void(float, float)> const& commitAction);
bool PropertiesInput(char const* label, char const* text, std::function<void(char const*)> const& changeAction, std::function<void(char const*, char const*)> const& commitAction);
bool PropertiesInput(char const* label, char const* text, int lines, std::function<void(char const*)> const& changeAction, std::function<void(char const*, char const*)> const& commitAction);
bool PropertiesInput(char const* label, moth_ui::Color color, std::function<void(moth_ui::Color color)> const& changeAction, std::function<void(moth_ui::Color, moth_ui::Color)> const& commitAction);
bool PropertiesInput(char const* label, moth_ui::LayoutRect value, std::function<void(moth_ui::LayoutRect)> const& changeAction, std::function<void(moth_ui::LayoutRect, moth_ui::LayoutRect)> const& commitAction);
bool PropertiesInput(char const* label, moth_ui::IntRect value, std::function<void(moth_ui::IntRect)> const& changeAction, std::function<void(moth_ui::IntRect, moth_ui::IntRect)> const& commitAction);
template <typename T, typename F = std::function<void(T, T)>, std::enable_if_t<std::is_enum_v<T>, bool> = true>
inline bool PropertiesInput(char const* label, T value, F const& changeAction) {
    bool changed = false;
    std::string const enumValueStr(magic_enum::enum_name(value));
    if (ImGui::BeginCombo(label, enumValueStr.c_str())) {
        for (size_t i = 0; i < magic_enum::enum_count<T>(); ++i) {
            auto const currentEnumValue = magic_enum::enum_value<T>(i);
            bool selected = currentEnumValue == value;
            std::string const currentValueStr(magic_enum::enum_name(currentEnumValue));
            if (ImGui::Selectable(currentValueStr.c_str(), selected)) {
                changeAction(value, currentEnumValue);
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}
template<typename T>
inline bool PropertiesInput(char const* label, T const& itemList, std::string const& currentValue, std::function<void(std::string const&, std::string const&)> const& changeAction) {
    bool changed = false;
    if (ImGui::BeginCombo(label, currentValue.c_str())) {
        for (auto item : itemList) {
            bool selected = item == currentValue;
            if (ImGui::Selectable(item.c_str(), selected)) {
                changeAction(currentValue, item);
                changed = true;
            }
        }
        ImGui::EndCombo();
    }
    return changed;
}
void PropertiesInputClear();