#pragma once

#include "moth_ui/event_listener.h"
#include "moth_ui/events/event_mouse.h"

class EditorPanelCanvas;
class BoundsHandle;

class BoundsWidget : public moth_ui::EventListener {
public:
    BoundsWidget(EditorPanelCanvas& canvasPanel);
    ~BoundsWidget();

    void BeginEdit();
    void EndEdit();

    bool OnEvent(moth_ui::Event const& event) override;
    void Draw();

    void SetSelection(std::shared_ptr<moth_ui::Node> node);
    std::shared_ptr<moth_ui::Node> GetSelection() const { return m_node; }

    moth_ui::IntVec2 SnapToGrid(moth_ui::IntVec2 const& original);

    EditorPanelCanvas& GetCanvasPanel() const { return m_canvasPanel; }

private:
    EditorPanelCanvas& m_canvasPanel;
    std::shared_ptr<moth_ui::Node> m_node;
    std::array<std::unique_ptr<BoundsHandle>, 16> m_handles;

    int m_anchorButtonSize = 12;
    int m_anchorButtonSpacing = 4;

    moth_ui::FloatRect m_anchorButtonTL;
    moth_ui::FloatRect m_anchorButtonFill;
    bool m_anchorTLPressed = false;
    bool m_anchorFillPressed = false;

    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
};
