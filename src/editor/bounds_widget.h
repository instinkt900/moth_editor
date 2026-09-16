#pragma once

#include "moth/ui/events/event_listener.h"
#include "moth/ui/events/event_mouse.h"
#include "moth/ui/utils/rect.h"

#include "bounds_handle.h"

class EditorPanelCanvas;
class PivotBoundsHandle;
class RotationBoundsHandle;

class BoundsWidget : public moth::ui::IEventListener {
public:
    BoundsWidget(EditorPanelCanvas& canvasPanel);
    ~BoundsWidget() override;

    void BeginEdit();
    void EndEdit();

    bool OnEvent(moth::ui::Event const& event) override;
    void Draw();

    void SetSelection(std::shared_ptr<moth::ui::Node> node);
    std::shared_ptr<moth::ui::Node> GetSelection() const { return m_node; }

    moth::ui::IntVec2 SnapToGrid(moth::ui::IntVec2 const& original);

    // Returns worldPos rotated around the current node's pivot
    moth::ui::FloatVec2 GetRotatedWorldPos(moth::ui::FloatVec2 const& worldPos) const;
    // Returns the world-space position of an anchor point on the node's screen rect, rotated
    moth::ui::FloatVec2 GetNodeAnchorWorldPos(BoundsHandleAnchor const& anchor) const;

    EditorPanelCanvas& GetCanvasPanel() const { return m_canvasPanel; }

private:
    EditorPanelCanvas& m_canvasPanel;
    std::shared_ptr<moth::ui::Node> m_node;
    std::array<std::unique_ptr<BoundsHandle>, 16> m_handles;
    std::array<std::unique_ptr<RotationBoundsHandle>, 4> m_rotationHandles;
    std::unique_ptr<PivotBoundsHandle> m_pivotHandle;

    int m_anchorButtonSize = 12;
    int m_anchorButtonSpacing = 4;

    moth::ui::FloatRect m_anchorButtonTL;
    moth::ui::FloatRect m_anchorButtonFill;
    bool m_anchorTLPressed = false;
    bool m_anchorFillPressed = false;

    bool OnMouseDown(moth::ui::EventMouseDown const& event);
    bool OnMouseUp(moth::ui::EventMouseUp const& event);
};
