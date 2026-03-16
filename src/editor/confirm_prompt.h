#pragma once

#include <functional>
#include <string>

class ConfirmPrompt {
public:
    void SetTitle(std::string const& title);
    void SetMessage(std::string const& message);
    void SetPositiveText(std::string const& text);
    void SetNegativeText(std::string const& text);
    void SetCancelText(std::string const& text);
    void SetPositiveAction(std::function<void()> const& action);
    void SetNegativeAction(std::function<void()> const& action);
    void SetCancelAction(std::function<void()> const& action);
    void Open();

    void Draw();

private:
    std::string m_title;
    std::string m_message;
    std::string m_positiveText = "OK";
    std::string m_negativeText = "Cancel";
    std::string m_cancelText;
    std::function<void()> m_positiveAction;
    std::function<void()> m_negativeAction;
    std::function<void()> m_cancelAction;
    bool m_pendingOpen = false;
};
