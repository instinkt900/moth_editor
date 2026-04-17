#pragma once

#include "editor_panel.h"
#include "../editor_config.h"
#include "../confirm_prompt.h"

class EditorPanelConfig : public EditorPanel {
public:
    EditorPanelConfig(EditorLayer& editorLayer, bool visible);
    ~EditorPanelConfig() override = default;

private:
    EditorConfig& m_config;
    ConfirmPrompt m_resetConfirm;

    char m_newResName[64] = {};
    int m_newResWidth = 0;
    int m_newResHeight = 0;

    bool BeginPanel() override;
    void DrawContents() override;
};
