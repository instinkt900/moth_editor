#pragma once

#include "editor_action.h"
#include "moth_ui/text_alignment.h"

class ChangeHAlignmentAction : public IEditorAction {
public:
    ChangeHAlignmentAction(std::shared_ptr<moth_ui::Node> node, moth_ui::TextHorizAlignment oldAlignment, moth_ui::TextHorizAlignment newAlignment);
    virtual ~ChangeHAlignmentAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    moth_ui::TextHorizAlignment m_oldAlignment;
    moth_ui::TextHorizAlignment m_newAlignment;
};
