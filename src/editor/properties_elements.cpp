#include "common.h"
#include "properties_elements.h"
#include "moth_ui/layout/layout_types.h"

namespace {
    ImGuiID editingElementID = {};
    std::vector<char> viewBuffer;
    std::vector<char> editBuffer;
    std::string preEditText;
    int preEditInt;
    int editInt;
    float preEditFloat;
    float editFloat;
    moth_ui::Color preEditColor;
    moth_ui::Color editColor;
    moth_ui::LayoutRect preEditLayoutRect;
    moth_ui::LayoutRect editLayoutRect;
    moth_ui::IntRect preEditRect;
    moth_ui::IntRect editRect;
    bool preEditTextSet = false;
    std::function<void(char const*, char const*)> editCompleteCallback;
    std::function<void(int, int)> commitIntAction;
    std::function<void(float, float)> commitFloatAction;
    std::function<void(moth_ui::Color, moth_ui::Color)> commitColorAction;
    std::function<void(moth_ui::LayoutRect, moth_ui::LayoutRect)> commitLayoutRectAction;
    std::function<void(moth_ui::IntRect, moth_ui::IntRect)> commitRectAction;

    void FillBuffer(char const* text, std::vector<char>& buffer) {
        auto const textLength = std::strlen(text) + 2; // two extra chars for a null and an input
        size_t newBufferSize = 256;
        while (textLength > newBufferSize) {
            newBufferSize *= 2;
        }
        if (newBufferSize > buffer.size()) {
            buffer.resize(newBufferSize);
        }
        for (int i = 0; i < textLength; ++i) {
            buffer[i] = text[i];
        }
        buffer[textLength] = '\0';
    }

    void BeginEdit(ImGuiID id, char const* initialText, std::function<void(char const*, char const*)> const& onEditComplete) {
        editingElementID = id;
        preEditText = initialText;
        FillBuffer(initialText, editBuffer);
        editCompleteCallback = onEditComplete;
    }

    void EndEdit() {
        editingElementID = {};
        if (editCompleteCallback) {
            editCompleteCallback(preEditText.c_str(), editBuffer.data());
        }
        if (commitColorAction) {
            commitColorAction(preEditColor, editColor);
        }
        if (commitLayoutRectAction) {
            commitLayoutRectAction(preEditLayoutRect, editLayoutRect);
        }
        if (commitRectAction) {
            commitRectAction(preEditRect, editRect);
        }
        if (commitFloatAction) {
            commitFloatAction(preEditFloat, editFloat);
        }
        if (commitIntAction) {
            commitIntAction(preEditInt, editInt);
        }
        editCompleteCallback = nullptr;
        commitColorAction = nullptr;
        commitRectAction = nullptr;
        commitFloatAction = nullptr;
        commitIntAction = nullptr;
    }

    std::vector<char>& GetBuffer(ImGuiID id, char const* currentText) {
        if (id == editingElementID) {
            return editBuffer;
        }
        FillBuffer(currentText, viewBuffer);
        return viewBuffer;
    }

    void BeginEditInt(ImGuiID id, int initialValue, std::function<void(int, int)> const& commitAction) {
        editingElementID = id;
        preEditInt = initialValue;
        commitIntAction = commitAction;
    }

    void BeginEditFloat(ImGuiID id, float initialValue, std::function<void(float, float)> const& commitAction) {
        editingElementID = id;
        preEditFloat = initialValue;
        commitFloatAction = commitAction;
    }

    void BeginEditColor(ImGuiID id, moth_ui::Color initialColor, std::function<void(moth_ui::Color, moth_ui::Color)> const& commitAction) {
        editingElementID = id;
        preEditColor = initialColor;
        commitColorAction = commitAction;
    }

    void BeginEditLayoutRect(ImGuiID id, moth_ui::LayoutRect initialRect, std::function<void(moth_ui::LayoutRect, moth_ui::LayoutRect)> const& commitAction) {
        editingElementID = id;
        preEditLayoutRect = initialRect;
        commitLayoutRectAction = commitAction;
    }

    void BeginEditRect(ImGuiID id, moth_ui::IntRect initialRect, std::function<void(moth_ui::IntRect, moth_ui::IntRect)> const& commitAction) {
        editingElementID = id;
        preEditRect = initialRect;
        commitRectAction = commitAction;
    }
}

bool PropertiesInput(char const* label, bool current, std::function<void(bool)> const& commitAction) {
    auto const changed = ImGui::Checkbox(label, &current);
    if (changed) {
        commitAction(current);
    }
    return changed;
}

bool PropertiesInput(char const* label, int current, std::function<void(int)> const& changeAction, std::function<void(int, int)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto const initialValue = current;
    editInt = current;
    auto const changed = ImGui::InputInt(label, &editInt);
    if (changed) {
        if (editingElementID != thisID) {
            BeginEditInt(thisID, initialValue, commitAction);
        }
        if (changeAction) {
            changeAction(editInt);
        }
    }
    auto const hasFocus = ImGui::IsItemFocused();
    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }
    return changed;
}

bool PropertiesInput(char const* label, float current, std::function<void(float)> const& changeAction, std::function<void(float, float)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto const initialValue = current;
    editFloat = current;
    auto const changed = ImGui::InputFloat(label, &editFloat);
    if (changed) {
        if (editingElementID != thisID) {
            BeginEditFloat(thisID, initialValue, commitAction);
        }
        if (changeAction) {
            changeAction(editFloat);
        }
    }
    auto const hasFocus = ImGui::IsItemFocused();
    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }
    return changed;
}

bool PropertiesInput(char const* label, char const* text, std::function<void(char const*)> const& changeAction, std::function<void(char const*, char const*)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto& buffer = GetBuffer(thisID, text);
    auto const changed = ImGui::InputText(label, buffer.data(), buffer.size() - 1);
    if (changed) {
        if (editingElementID != thisID) {
            BeginEdit(thisID, text, commitAction);
        }
        if (changeAction) {
            changeAction(buffer.data());
        }
    }
    auto const hasFocus = ImGui::IsItemFocused();
    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }
    return changed;
}

bool PropertiesInput(char const* label, char const* text, int lines, std::function<void(char const*)> const& changeAction, std::function<void(char const*, char const*)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto& buffer = GetBuffer(thisID, text);
    auto const changed = ImGui::InputTextMultiline(label, buffer.data(), buffer.size() - 1, ImVec2{ 0, lines * ImGui::GetFontSize() });
    if (changed) {
        if (editingElementID != thisID) {
            BeginEdit(thisID, text, commitAction);
        }
        if (changeAction) {
            changeAction(buffer.data());
        }
    }
    auto const hasFocus = ImGui::IsItemFocused();
    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }
    return changed;
}

bool PropertiesInput(char const* label, moth_ui::Color color, std::function<void(moth_ui::Color color)> const& changeAction, std::function<void(moth_ui::Color, moth_ui::Color)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto const initialColor = color;
    editColor = color;
    auto const changed = ImGui::ColorEdit4(label, editColor.data, 0);
    if (changed) {
        if (editingElementID != thisID) {
            BeginEditColor(thisID, initialColor, commitAction);
        }
        if (changeAction) {
            changeAction(editColor);
        }
    }
    auto const hasFocus = ImGui::IsItemFocused();
    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }
    return changed;
}

bool PropertiesInput(char const* label, moth_ui::LayoutRect value, std::function<void(moth_ui::LayoutRect)> const& changeAction, std::function<void(moth_ui::LayoutRect, moth_ui::LayoutRect)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto const initialValue = value;
    editLayoutRect = value;
    bool changed = false;
    bool hasFocus = false;

    if (ImGui::CollapsingHeader(label)) {
        if (ImGui::TreeNode("Anchor")) {
            ImGui::PushItemWidth(50);
            changed |= ImGui::InputFloat("Top", &editLayoutRect.anchor.topLeft.y, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            changed |= ImGui::InputFloat("Left", &editLayoutRect.anchor.topLeft.x, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            changed |= ImGui::InputFloat("Bottom", &editLayoutRect.anchor.bottomRight.y, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            changed |= ImGui::InputFloat("Right", &editLayoutRect.anchor.bottomRight.x, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            ImGui::PopItemWidth();
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Offset")) {
            ImGui::PushItemWidth(50);
            changed |= ImGui::InputFloat("Top", &editLayoutRect.offset.topLeft.y, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            changed |= ImGui::InputFloat("Left", &editLayoutRect.offset.topLeft.x, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            changed |= ImGui::InputFloat("Bottom", &editLayoutRect.offset.bottomRight.y, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            changed |= ImGui::InputFloat("Right", &editLayoutRect.offset.bottomRight.x, 0, 0, "%.2f");
            hasFocus |= ImGui::IsItemFocused();
            ImGui::PopItemWidth();
            ImGui::TreePop();
        }
    }

    if (changed) {
        if (editingElementID != thisID) {
            BeginEditLayoutRect(thisID, initialValue, commitAction);
        }
        if (changeAction) {
            changeAction(editLayoutRect);
        }
    }

    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }

    return changed;
}

bool PropertiesInput(char const* label, moth_ui::IntRect value, std::function<void(moth_ui::IntRect)> const& changeAction, std::function<void(moth_ui::IntRect, moth_ui::IntRect)> const& commitAction) {
    auto const thisID = ImGui::GetID(label);
    auto const initialValue = value;
    editRect = value;
    bool changed = false;
    bool hasFocus = false;

    if (ImGui::CollapsingHeader(label)) {
        changed |= ImGui::InputInt("Top", &editRect.topLeft.y);
        hasFocus |= ImGui::IsItemFocused();
        changed |= ImGui::InputInt("Left", &editRect.topLeft.x);
        hasFocus |= ImGui::IsItemFocused();
        changed |= ImGui::InputInt("Bottom", &editRect.bottomRight.y);
        hasFocus |= ImGui::IsItemFocused();
        changed |= ImGui::InputInt("Right", &editRect.bottomRight.x);
        hasFocus |= ImGui::IsItemFocused();
    }

    if (changed) {
        if (editingElementID != thisID) {
            BeginEditRect(thisID, initialValue, commitAction);
        }
        if (changeAction) {
            changeAction(editRect);
        }
    }

    if (editingElementID == thisID && !hasFocus) {
        EndEdit();
    }

    return changed;
}

void PropertiesInputClear() {
    EndEdit();
}
