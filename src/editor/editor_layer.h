#pragma once

#include "layers/layer.h"
#include "moth_ui/layout/layout_entity_group.h"
#include "moth_ui/group.h"
#include "moth_ui/events/event_mouse.h"
#include "moth_ui/events/event_key.h"

#include "imgui.h"
#include "imgui-filebrowser/imfilebrowser.h"

class BoundsWidget;
class AnimationWidget;
class PropertiesEditor;
class IEditorAction;
class ChangeBoundsAction;

class EditorLayer : public Layer {
public:
    EditorLayer();
    virtual ~EditorLayer();

    bool OnEvent(moth_ui::Event const& event) override;

    void Update(uint32_t ticks) override;
    void Draw(SDL_Renderer& renderer) override;
    void DebugDraw() override;

    void OnAddedToStack(LayerStack* layerStack) override;
    void OnRemovedFromStack() override;

    bool UseRenderSize() const override { return false; }

    void AddEditAction(std::unique_ptr<IEditorAction>&& editAction);

    void SetSelectedFrame(int frameNo);
    int GetSelectedFrame() const { return m_selectedFrame; }

    moth_ui::Group* GetRoot() const { return m_root.get(); }
    void SetSelection(std::shared_ptr<moth_ui::Node> selection);
    std::shared_ptr<moth_ui::Node> GetSelection() const { return m_selection; }
    bool IsSelected(std::shared_ptr<moth_ui::Node> node) const { return m_selection == node; }

    void Refresh();

    void BeginEditBounds();
    void EndEditBounds();

    auto GetScaleFactor() const { return 100.0f / m_displayZoom; }
    bool SnapToGrid() const { return m_gridSpacing > 0; }
    auto const& GetCanvasTopLeft() const { return m_canvasTopLeft; }
    auto GetGridSpacing() const { return m_gridSpacing; }

private:
    enum class FileOpenMode {
        Unknown,
        Layout,
        SubLayout,
        Image,
        Save,
    };
    ImGui::FileBrowser m_fileDialog;
    FileOpenMode m_fileOpenMode = FileOpenMode::Unknown;
    std::shared_ptr<moth_ui::LayoutEntityGroup> m_rootLayout;
    std::shared_ptr<moth_ui::Group> m_root;
    std::shared_ptr<moth_ui::Node> m_selection;
    int m_selectedFrame = 0;

    int m_displayZoom = 100;
    static int constexpr s_maxZoom = 800;
    static int constexpr s_minZoom = 30;
    moth_ui::IntVec2 m_displaySize{ 200, 200 };
    moth_ui::FloatVec2 m_canvasOffset{ 0, 0 };
    bool m_canvasGrabbed = false;
    int m_gridSpacing = 5;
    moth_ui::IntVec2 m_canvasTopLeft;

    std::vector<std::unique_ptr<IEditorAction>> m_editActions;
    int m_actionIndex = -1;

    std::unique_ptr<BoundsWidget> m_boundsWidget;
    std::unique_ptr<AnimationWidget> m_animationWidget;
    std::unique_ptr<PropertiesEditor> m_propertiesEditor;

    bool m_visibleCanvasProperties = true;
    bool m_visiblePropertiesPanel = true;
    bool m_visibleAnimationPanel = true;
    bool m_visibleElementsPanel = true;
    bool m_visibleUndoPanel = false;

    void DrawMainMenu();
    void DrawCanvasProperties();
    void DrawPropertiesPanel();
    void DrawElementsPanel();
    void DrawAnimationPanel();
    void DrawUndoStack();
    void DrawCanvas(SDL_Renderer& renderer);

    void UndoEditAction();
    void RedoEditAction();
    void ClearEditActions();

    void NewLayout();
    void LoadLayout(char const* path);
    void SaveLayout(char const* path);
    void AddSubLayout(char const* path);
    void AddImage(char const* path);
    void Rebuild();

    bool OnKey(moth_ui::EventKey const& event);
    bool OnMouseDown(moth_ui::EventMouseDown const& event);
    bool OnMouseUp(moth_ui::EventMouseUp const& event);
    bool OnMouseMove(moth_ui::EventMouseMove const& event);
    bool OnMouseWheel(moth_ui::EventMouseWheel const& event);

    std::unique_ptr<moth_ui::Event> AlterMouseEvents(moth_ui::Event const& inEvent);

    struct EditBoundsContext {
        std::shared_ptr<moth_ui::LayoutEntity> entity;
        moth_ui::LayoutRect originalRect;
    };
    std::unique_ptr<EditBoundsContext> m_editBoundsContext;
};
