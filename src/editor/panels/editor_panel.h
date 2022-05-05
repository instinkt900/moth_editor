#pragma once

#include "moth_ui/event_listener.h"

class EditorLayer;

enum class EditorPanelType {
    CanvasProperties,
    ProjectProperties,
    LayoutList,
    ImageList,
    Properties,
    Elements,
    Animation,
    Keyframes,
    UndoStack,
    Preview,
};

class EditorPanel : public moth_ui::EventListener {
public:
    EditorPanel(EditorLayer& editorLayer, std::string const& title, bool visible, bool exposed);
    virtual ~EditorPanel() = default;

    std::string const& GetTitle() const { return m_title; }
    bool IsExposed() const { return m_exposed; }

    virtual void OnLayoutLoaded() {}
    virtual void OnShutdown() {}

    virtual void Refresh() {}

    virtual bool OnEvent(moth_ui::Event const& event) { return false; }
    virtual void Update(uint32_t ticks) {}
    void Draw();

    bool m_visible = false;

protected:
    EditorLayer& m_editorLayer;
    std::string m_title;
    bool m_exposed = false;
    bool m_wasVisible = false;

    virtual void OnOpen() {}
    virtual void OnClose() {}
    virtual bool BeginPanel();
    virtual void DrawContents() = 0;
    virtual void EndPanel();
};
