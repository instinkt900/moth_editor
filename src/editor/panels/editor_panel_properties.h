#pragma once

#include "editor_panel.h"
#include "moth/ui/context.h"
#include "moth/ui/layout/layout_rect.h"

class EditorPanelProperties : public EditorPanel {
public:
    EditorPanelProperties(EditorLayer& editorLayer, bool visible);
    ~EditorPanelProperties() override = default;

    void OnLayoutLoaded() override;

private:
    bool BeginPanel() override;
    void EndPanel() override;
    void DrawContents() override;

    std::shared_ptr<moth::ui::Node> m_currentSelection = nullptr;
    std::shared_ptr<moth::ui::Node> m_lastSelection = nullptr;
    std::optional<moth::ui::LayoutRect> m_boundsClipboard;

    void DrawNodeProperties(std::shared_ptr<moth::ui::Node> node, bool recurseChildren = true);
    void DrawCommonProperties(std::shared_ptr<moth::ui::Node> node);
    void DrawBoundsTools(std::shared_ptr<moth::ui::Node> node);
    void DrawRectProperties(std::shared_ptr<moth::ui::NodeRect> node);
    void DrawImageProperties(std::shared_ptr<moth::ui::NodeImage> node);
    void DrawTextProperties(std::shared_ptr<moth::ui::NodeText> node);
    void DrawFlipbookProperties(std::shared_ptr<moth::ui::NodeFlipbook> node);
    void DrawGradientProperties(std::shared_ptr<moth::ui::NodeGradient> node);
    void DrawRefProperties(std::shared_ptr<moth::ui::Group> node, bool recurseChildren);
    void DrawLayoutProperties(std::shared_ptr<moth::ui::Group> node);
};
