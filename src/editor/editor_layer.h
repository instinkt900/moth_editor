#pragma once

#include "layers/layer.h"
#include "uilib/layout/layout_entity_group.h"
#include "uilib/group.h"
#include "uilib/events/event_mouse.h"
#include "uilib/events/event_key.h"

#include "imgui.h"
#include "imgui-filebrowser/imfilebrowser.h"

namespace ui {
    class BoundsWidget;
    class AnimationWidget;
    class PropertiesEditor;
    class IEditorAction;
    class ChangeBoundsAction;

    class EditorLayer : public Layer {
    public:
        EditorLayer();
        virtual ~EditorLayer();

        bool OnEvent(Event const& event) override;

        void Update(uint32_t ticks) override;
        void Draw(SDL_Renderer& renderer) override;
        void DebugDraw() override;

        void OnAddedToStack(LayerStack* layerStack) override;
        void OnRemovedFromStack() override;

        bool UseRenderSize() const override { return false; }

        void AddEditAction(std::unique_ptr<IEditorAction>&& editAction);

        void SetSelectedFrame(int frameNo);
        int GetSelectedFrame() const { return m_selectedFrame; }

        Group* GetRoot() const { return m_root.get(); }
        void SetSelection(std::shared_ptr<Node> selection);
        std::shared_ptr<Node> GetSelection() const { return m_selection; }
        bool IsSelected(std::shared_ptr<Node> node) const { return m_selection == node; }

        void Refresh();

        void BeginEditBounds();
        void EndEditBounds();

        auto GetScaleFactor() const { return 100.0f / m_displayZoom; }
        bool SnapToGrid() const { return m_gridSpacing > 0; }
        auto const& GetCanvasTopLeft() const { return m_canvasTopLeft; }
        auto GetGridSpacing() const { return m_gridSpacing;}

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
        std::shared_ptr<LayoutEntityGroup> m_rootLayout;
        std::shared_ptr<Group> m_root;
        std::shared_ptr<Node> m_selection;
        int m_selectedFrame = 0;

        int m_displayZoom = 100;
        static int constexpr s_maxZoom = 800;
        static int constexpr s_minZoom = 30;
        IntVec2 m_displaySize{ 200, 200 };
        FloatVec2 m_canvasOffset{ 0, 0 };
        bool m_canvasGrabbed = false;
        int m_gridSpacing = 5;
        IntVec2 m_canvasTopLeft;

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

        bool OnKey(EventKey const& event);
        bool OnMouseDown(EventMouseDown const& event);
        bool OnMouseUp(EventMouseUp const& event);
        bool OnMouseMove(EventMouseMove const& event);
        bool OnMouseWheel(EventMouseWheel const& event);

        std::unique_ptr<Event> AlterMouseEvents(Event const& inEvent);

        struct EditBoundsContext {
            std::shared_ptr<LayoutEntity> entity;
            LayoutRect originalRect;
        };
        std::unique_ptr<EditBoundsContext> m_editBoundsContext;
    };
};
