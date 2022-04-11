#pragma once

#include "editor_action.h"
#include "moth_ui/image_scale_type.h"

class SetImageScaleTypeAction : public IEditorAction {
public:
    SetImageScaleTypeAction(std::shared_ptr<moth_ui::Node> node, moth_ui::ImageScaleType oldScaleType, moth_ui::ImageScaleType newScaleType);
    virtual ~SetImageScaleTypeAction();

    void Do() override;
    void Undo() override;

    void OnImGui() override;

protected:
    std::shared_ptr<moth_ui::Node> m_node;
    moth_ui::ImageScaleType m_oldScaleType;
    moth_ui::ImageScaleType m_newScaleType;
};
