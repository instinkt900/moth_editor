#include "common.h"
#include "confirm_prompt.h"

void ConfirmPrompt::SetTitle(std::string const& title) {
    m_title = title;
}

void ConfirmPrompt::SetMessage(std::string const& message) {
    m_message = message;
}

void ConfirmPrompt::SetPositiveText(std::string const& text) {
    m_positiveText = text;
}

void ConfirmPrompt::SetNegativeText(std::string const& text) {
    m_negativeText = text;
}

void ConfirmPrompt::SetPositiveAction(std::function<void()> const& action) {
    m_positiveAction = action;
}

void ConfirmPrompt::SetNegativeAction(std::function<void()> const& action) {
    m_negativeAction = action;
}

void ConfirmPrompt::Open() {
    m_pendingOpen = true;
}

void ConfirmPrompt::Draw() {
    if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(m_message.c_str());
        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if (ImGui::Button(m_positiveText.c_str(), button_size)) {
            if (m_positiveAction) {
                m_positiveAction();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(m_negativeText.c_str(), button_size)) {
            if (m_negativeAction) {
                m_negativeAction();
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (m_pendingOpen) {
        m_pendingOpen = false;
        ImGui::OpenPopup(m_title.c_str());
    }
}
