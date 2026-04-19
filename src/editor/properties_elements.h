#pragma once

#include "moth_ui/layout/layout_rect.h"

class PropertyEditContextBase {
public:
    PropertyEditContextBase(ImGuiID id)
        : m_id(id) {}
    virtual ~PropertyEditContextBase() = default;
    ImGuiID GetID() const { return m_id; }
    virtual void Commit() = 0;

protected:
    ImGuiID m_id;
};
template <class T>
class PropertyEditContext : public PropertyEditContextBase {
public:
    PropertyEditContext(ImGuiID id, T const& originalValue, std::function<void(T, T)> const& commitAction)
        : PropertyEditContextBase(id)
        , m_originalValue(originalValue)
        , m_pendingValue(originalValue)
        , m_commitAction(commitAction) {
    }
    ~PropertyEditContext() override = default;

    void UpdateValue(T const& value) {
        m_pendingValue = value;
    }

    void Commit() override {
        if (m_originalValue != m_pendingValue) {
            m_commitAction(m_originalValue, m_pendingValue);
        }
    }

private:
    T m_originalValue;
    T m_pendingValue;
    std::function<void(T, T)> m_commitAction;
};

template <>
class PropertyEditContext<char const*> : public PropertyEditContextBase {
public:
    PropertyEditContext(ImGuiID id, char const* const& originalValue, std::function<void(char const*, char const*)> const& commitAction)
        : PropertyEditContextBase(id)
        , m_originalValue(originalValue)
        , m_pendingValue(originalValue)
        , m_commitAction(commitAction) {
    }
    ~PropertyEditContext() override = default;

    void UpdateValue(char const* const& value) {
        m_pendingValue = value;
    }

    void Commit() override {
        if (m_originalValue != m_pendingValue) {
            m_commitAction(m_originalValue.c_str(), m_pendingValue.c_str());
        }
    }

private:
    std::string m_originalValue;
    std::string m_pendingValue;
    std::function<void(char const*, char const*)> m_commitAction;
};

extern std::unique_ptr<PropertyEditContextBase> m_currentEditContext;
extern ImGuiID m_editingID;

ImGuiID GetCurrentEditFocusID();
void CommitEditContext();
void BeginEdits();
void EndEdits();

template <class SourceType>
struct InputBuffer {
    SourceType* Buffer;
    size_t Size;
};

template <>
struct InputBuffer<char const*> {
    char* Buffer;
    size_t Size;
};

template <class SourceType>
InputBuffer<SourceType> GetBufferForValue(SourceType const& value) {
    static SourceType buffer;
    buffer = value;
    return { &buffer, sizeof(SourceType) };
}

inline InputBuffer<char const*> GetBufferForValue(char const* const& value) {
    static std::vector<char> buffer;
    auto const valueLen = strlen(value);
    auto const wantedSize = valueLen + 256;
    if (buffer.size() < wantedSize) {
        buffer.resize(wantedSize);
    }
    for (size_t i = 0; i < valueLen; ++i) {
        buffer[i] = value[i];
    }
    buffer[valueLen] = 0;
    return { buffer.data(), buffer.size() };
}

template <class SourceType>
struct InputContext {
    bool Changed;
    bool Focused;
    InputBuffer<SourceType> ValueBuffer;
};

inline InputContext<bool> InputElement(char const* label, InputBuffer<bool> valueBuffer) {
    bool changed = ImGui::Checkbox(label, valueBuffer.Buffer);
    bool focused = false;
    return { changed, focused, valueBuffer };
}

inline InputContext<int> InputElement(char const* label, InputBuffer<int> valueBuffer) {
    bool changed = ImGui::InputInt(label, valueBuffer.Buffer, 0);
    bool focused = ImGui::IsItemFocused();
    return { changed, focused, valueBuffer };
}

inline InputContext<float> InputElement(char const* label, InputBuffer<float> valueBuffer) {
    bool changed = ImGui::InputFloat(label, valueBuffer.Buffer);
    bool focused = ImGui::IsItemFocused();
    return { changed, focused, valueBuffer };
}

inline InputContext<char const*> InputElement(char const* label, InputBuffer<char const*> valueBuffer) {
    bool changed = ImGui::InputText(label, valueBuffer.Buffer, valueBuffer.Size - 1);
    bool focused = ImGui::IsItemFocused();
    return { changed, focused, valueBuffer };
}

inline InputContext<moth_ui::IntVec2> InputElement(char const* label, InputBuffer<moth_ui::IntVec2> valueBuffer) {
    bool changed = ImGui::InputInt2(label, valueBuffer.Buffer->data, 0);
    bool focused = ImGui::IsItemFocused();
    return { changed, focused, valueBuffer };
}

inline InputContext<moth_ui::FloatVec2> InputElement(char const* label, InputBuffer<moth_ui::FloatVec2> valueBuffer) {
    bool changed = ImGui::InputFloat2(label, valueBuffer.Buffer->data, "%.3f");
    bool focused = ImGui::IsItemFocused();
    return { changed, focused, valueBuffer };
}

inline InputContext<moth_ui::Color> InputElement(char const* label, InputBuffer<moth_ui::Color> valueBuffer) {
    bool changed = ImGui::ColorEdit4(label, valueBuffer.Buffer->data, 0);
    bool focused = ImGui::IsItemFocused();
    return { changed, focused, valueBuffer };
}

inline InputContext<moth_ui::LayoutRect> InputElement(char const* label, InputBuffer<moth_ui::LayoutRect> valueBuffer) {
    bool changed = false;
    bool focused = false;

    ImGui::Text("%s", label);

    auto drawSection = [&](char const* header, moth_ui::FloatRect& rect) {
        ImGui::Indent();
        ImGui::Text("%s", header);
        ImGui::PushID(header);
        if (ImGui::BeginTable("##tbl", 2)) {
            ImGui::TableNextColumn();
            ImGui::Text("L");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed |= ImGui::InputFloat("##l", &rect.topLeft.x, 0, 0, "%.2f");
            focused |= ImGui::IsItemFocused();

            ImGui::TableNextColumn();
            ImGui::Text("T");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed |= ImGui::InputFloat("##t", &rect.topLeft.y, 0, 0, "%.2f");
            focused |= ImGui::IsItemFocused();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("R");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed |= ImGui::InputFloat("##r", &rect.bottomRight.x, 0, 0, "%.2f");
            focused |= ImGui::IsItemFocused();

            ImGui::TableNextColumn();
            ImGui::Text("B");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            changed |= ImGui::InputFloat("##b", &rect.bottomRight.y, 0, 0, "%.2f");
            focused |= ImGui::IsItemFocused();

            ImGui::EndTable();
        }
        ImGui::PopID();
        ImGui::Unindent();
    };

    drawSection("Anchor", valueBuffer.Buffer->anchor);
    drawSection("Offset", valueBuffer.Buffer->offset);

    return { changed, focused, valueBuffer };
}

inline InputContext<moth_ui::IntRect> InputElement(char const* label, InputBuffer<moth_ui::IntRect> valueBuffer) {
    bool changed = false;
    bool focused = false;

    if (ImGui::CollapsingHeader(label)) {
        changed |= ImGui::InputInt("Top", &valueBuffer.Buffer->topLeft.y, 0);
        focused |= ImGui::IsItemFocused();
        changed |= ImGui::InputInt("Left", &valueBuffer.Buffer->topLeft.x, 0);
        focused |= ImGui::IsItemFocused();
        changed |= ImGui::InputInt("Bottom", &valueBuffer.Buffer->bottomRight.y, 0);
        focused |= ImGui::IsItemFocused();
        changed |= ImGui::InputInt("Right", &valueBuffer.Buffer->bottomRight.x, 0);
        focused |= ImGui::IsItemFocused();
    }

    return { changed, focused, valueBuffer };
}

template <typename T, typename F = std::function<void(T, T)>, std::enable_if_t<std::is_enum_v<T>, bool> = true>
InputContext<T> InputElement(char const* label, InputBuffer<T> valueBuffer) {
    bool changed = false;
    bool focused = false;

    std::string const enumValueStr(magic_enum::enum_name(*valueBuffer.Buffer));
    if (ImGui::BeginCombo(label, enumValueStr.c_str())) {
        for (size_t i = 0; i < magic_enum::enum_count<T>(); ++i) {
            auto const currentEnumValue = magic_enum::enum_value<T>(i);
            using UnderlyingT = std::underlying_type_t<T>;
            if constexpr (std::is_signed_v<UnderlyingT>) {
                if (static_cast<UnderlyingT>(currentEnumValue) < 0) {
                    continue;
                }
            }
            bool selected = currentEnumValue == *valueBuffer.Buffer;
            std::string const currentValueStr(magic_enum::enum_name(currentEnumValue));
            if (ImGui::Selectable(currentValueStr.c_str(), selected) && currentEnumValue != *valueBuffer.Buffer) {
                *valueBuffer.Buffer = currentEnumValue;
                changed = true;
                focused = true;
            }
        }
        ImGui::EndCombo();
    }

    return { changed, focused, valueBuffer };
}


template <class SourceType>
InputContext<SourceType> TypeInput(char const* label, SourceType value) {
    auto valueBuffer = GetBufferForValue(value);
    auto inputCtx = InputElement(label, valueBuffer);
    return inputCtx;
}

template <class SourceType>
void OnInputFocus(char const* label, SourceType const& value, std::function<void(SourceType, SourceType)> const& commitAction) {
    auto const id = ImGui::GetID(label);
    if (GetCurrentEditFocusID() != id) {
        CommitEditContext();
        m_currentEditContext = std::make_unique<PropertyEditContext<SourceType>>(id, value, commitAction);
    } else {
        auto* context = static_cast<PropertyEditContext<SourceType>*>(m_currentEditContext.get());
        context->UpdateValue(value);
    }
    m_editingID = id;
}

template <class T>
bool PropertiesInput(char const* label, T current, std::function<void(T)> const& changeAction = {}, std::function<void(T, T)> const& commitAction = {}) {
    auto const inputContext = TypeInput(label, current);
    if (changeAction && inputContext.Changed) {
        if constexpr (std::is_same_v<T, char const*>) {
            changeAction(inputContext.ValueBuffer.Buffer);
        } else {
            changeAction(*inputContext.ValueBuffer.Buffer);
        }
    }
    if (commitAction && inputContext.Focused) {
        if constexpr (std::is_enum_v<T>) {
            // Enum combos are instant-commit: selection completes in one frame so
            // Focused and Changed arrive together. The focus-tracking path would
            // capture the already-mutated value as originalValue and never see a
            // delta. Instead, commit immediately using the pre-mutation value
            // (current) and the newly selected value.
            if (inputContext.Changed) {
                CommitEditContext();
                commitAction(current, *inputContext.ValueBuffer.Buffer);
            }
        } else if constexpr (std::is_same_v<T, char const*>) {
            OnInputFocus<T>(label, inputContext.ValueBuffer.Buffer, commitAction);
        } else {
            OnInputFocus(label, *inputContext.ValueBuffer.Buffer, commitAction);
        }
    }
    return inputContext.Changed;
}

template <typename T>
bool PropertiesInputList(char const* label, T const& itemList, std::string const& currentValue, std::function<void(std::string const&, std::string const&)> const& changeAction) {
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

inline bool PropertiesInput(char const* label, char const* text, int lines, std::function<void(char const*)> const& changeAction, std::function<void(char const*, char const*)> const& commitAction) {
    auto valueBuffer = GetBufferForValue(text);
    // TODO: add ImGuiInputTextFlags_WordWrapping once imgui is upgraded to 1.91.0+
    bool const changed = ImGui::InputTextMultiline(label, valueBuffer.Buffer, valueBuffer.Size - 1, ImVec2{ 0, static_cast<float>(std::max(1, lines)) * ImGui::GetFontSize() });
    bool const focused = ImGui::IsItemFocused();

    if (changeAction && changed) {
        changeAction(valueBuffer.Buffer);
    }
    if (commitAction && focused) {
        OnInputFocus(label, static_cast<char const*>(valueBuffer.Buffer), commitAction);
    }
    return changed;
}
