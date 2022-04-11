#pragma once

#include "editor_action.h"
#include "moth_ui/text_alignment.h"

class ChangeVAlignmentAction : public IEditorAction {
public:
    ChangeVAlignmentAction(std::shared_ptr<moth_ui::Node> node, moth_ui::TextVertAlignment oldAlignment, moth_ui::TextVertAlignment newAlignment);
    virtual ~ChangeVAlignmentAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    moth_ui::TextVertAlignment m_oldAlignment;
    moth_ui::TextVertAlignment m_newAlignment;
};
