#pragma once

#include "editor_panel.h"
#include "../editor_config.h"
#include "../confirm_prompt.h"

class EditorPanelConfig : public EditorPanel {
public:
    EditorPanelConfig(EditorLayer& editorLayer, bool visible);
    virtual ~EditorPanelConfig() = default;

private:
    EditorConfig& m_config;
    ConfirmPrompt m_resetConfirm;

    void DrawContents() override;
};
