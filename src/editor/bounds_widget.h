#pragma once

#include "moth_ui/event_listener.h"
#include "moth_ui/events/event_mouse.h"

class EditorLayer;
class BoundsHandle;

class BoundsWidget : public moth_ui::EventListener {
public:
    BoundsWidget(EditorLayer& editorLayer);
    ~BoundsWidget();

    void BeginEdit();
    void EndEdit();

    bool OnEvent(moth_ui::Event const& event) override;
    void Draw(SDL_Renderer& renderer);

    EditorLayer& GetEditorLayer() { return m_editorLayer; }

    moth_ui::IntVec2 SnapToGrid(moth_ui::IntVec2 const& original);

private:
    EditorLayer& m_editorLayer;

    std::array<std::unique_ptr<BoundsHandle>, 16> m_handles;

    bool m_holding = false;
    moth_ui::IntVec2 m_grabPosition;

    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
    bool OnMouseMove(moth_ui::EventMouseMove const& event);

    void UpdatePosition(moth_ui::IntVec2 const& position);
};
