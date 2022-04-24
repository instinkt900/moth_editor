#pragma once

#include "layers/layer.h"
#include "events/event.h"
#include "layout_project.h"
#include "confirm_prompt.h"

#include "moth_ui/layout/layout_rect.h"
#include "moth_ui/events/event_mouse.h"
#include "moth_ui/events/event_key.h"

class BoundsWidget;
class IEditorAction;
class EditorPanel;

class EditorLayer : public Layer {
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    bool OnEvent(moth_ui::Event const& event) override;

    void Update(uint32_t ticks) override;
    void Draw(SDL_Renderer& renderer) override;
    void DebugDraw() override;

    void OnAddedToStack(LayerStack* layerStack) override;
    void OnRemovedFromStack() override;

    bool UseRenderSize() const override { return false; }

    struct CanvasProperties {
        moth_ui::IntVec2 m_size{ 640, 480 };
        moth_ui::FloatVec2 m_offset{ 0, 0 };
        int m_zoom = 100;
        int m_gridSpacing = 5;
    };

    void SetSelectedFrame(int frameNo);
    int GetSelectedFrame() const { return m_selectedFrame; }

    void SetSelection(std::shared_ptr<moth_ui::Node> selection);
    std::shared_ptr<moth_ui::Node> GetSelection() const { return m_selection; }
    bool IsSelected(std::shared_ptr<moth_ui::Node> node) const { return m_selection == node; }

    void Refresh();

    void BeginEditBounds();
    void EndEditBounds();

    void BeginEditColor();
    void EndEditColor();

    void PerformEditAction(std::unique_ptr<IEditorAction>&& editAction);
    void AddEditAction(std::unique_ptr<IEditorAction>&& editAction);
    int GetEditActionPos() const { return m_actionIndex; }

    LayoutProject& GetLayoutProject() { return m_layoutProject; }
    CanvasProperties& GetCanvasProperties() { return m_canvasProperties; }
    std::vector<std::unique_ptr<IEditorAction>> const& GetEditActions() const { return m_editActions; }

    float GetScaleFactor() const { return 100.0f / m_canvasProperties.m_zoom; }
    bool SnapToGrid() const { return m_canvasProperties.m_gridSpacing > 0; }
    moth_ui::IntVec2 const& GetCanvasTopLeft() const { return m_canvasTopLeft; }
    int GetGridSpacing() const { return m_canvasProperties.m_gridSpacing; }

    void NewLayout(bool discard = false);
    void LoadLayout(char const* path, bool discard = false);

    std::shared_ptr<moth_ui::Group> GetRoot() const { return m_root; }
    std::shared_ptr<moth_ui::Layout> GetCurrentLayout() { return m_rootLayout; }

    template <typename T, typename... Args>
    T* AddEditorPanel(Args&& ...args) {
        auto const id = typeid(T).hash_code();
        auto newPanel = std::make_unique<T>(std::forward<Args>(args)...);
        auto const newPanelPtr = newPanel.get();
        m_panels[id] = std::move(newPanel);
        return newPanelPtr;
    }

    template <typename T>
    T* GetEditorPanel() {
        auto const id = typeid(T).hash_code();
        auto const it = m_panels.find(id);
        if (std::end(m_panels) != it) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    template<typename T>
    void SetEditorPanelVisible(bool visible) {
        if (auto const panel = GetEditorPanel<T>()) {
            panel->m_visible = visible;
        }
    }

private:
    CanvasProperties m_canvasProperties;
    bool m_canvasGrabbed = false;
    moth_ui::IntVec2 m_canvasTopLeft;

    std::map<size_t, std::unique_ptr<EditorPanel>> m_panels;
    std::unique_ptr<BoundsWidget> m_boundsWidget;

    LayoutProject m_layoutProject;

    std::string m_currentLayoutPath;
    std::shared_ptr<moth_ui::Layout> m_rootLayout;
    std::shared_ptr<moth_ui::Group> m_root;
    std::shared_ptr<moth_ui::Node> m_selection;

    int m_selectedFrame = 0;

    struct EditBoundsContext {
        std::shared_ptr<moth_ui::LayoutEntity> entity;
        moth_ui::LayoutRect originalRect;
    };

    struct EditColorContext {
        std::shared_ptr<moth_ui::LayoutEntity> entity;
        moth_ui::Color originalColor;
    };

    std::unique_ptr<EditBoundsContext> m_editBoundsContext;
    std::unique_ptr<EditColorContext> m_editColorContext;
    std::vector<std::unique_ptr<IEditorAction>> m_editActions;
    int m_actionIndex = -1;
    int m_lastSaveActionIndex = -1;
    bool IsWorkPending() const { return m_lastSaveActionIndex != m_actionIndex; }
    ConfirmPrompt m_confirmPrompt;

    void DrawMainMenu();
    void DrawCanvas(SDL_Renderer& renderer);

    void UndoEditAction();
    void RedoEditAction();
    void ClearEditActions();

    void LoadProject(char const* path);
    void SaveProject(char const* path);
    void SaveLayout(char const* path);
    void Rebuild();

    void MoveSelectionUp();
    void MoveSelectionDown();

    std::shared_ptr<moth_ui::LayoutEntity> m_copiedEntity;
    void CopyEntity();
    void PasteEntity();

    bool OnKey(moth_ui::EventKey const& event);
    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
    bool OnMouseMove(moth_ui::EventMouseMove const& event);
    bool OnMouseWheel(moth_ui::EventMouseWheel const& event);
    bool OnRequestQuitEvent(EventRequestQuit const& event);

    std::unique_ptr<moth_ui::Event> AlterMouseEvents(moth_ui::Event const& inEvent);
};
