#pragma once

#include "moth_ui/context.h"
#include "moth_ui/layers/layer.h"
#include "moth_ui/events/event.h"
#include "confirm_prompt.h"
#include "editor_config.h"
#include "editor/texture_packer.h"
#include "editor/actions/editor_action.h"
#include "editor/panels/editor_panel.h"

#include "moth_ui/layout/layout_rect.h"
#include "moth_ui/events/event_mouse.h"
#include "moth_ui/events/event_key.h"

#include "moth_graphics/graphics/igraphics.h"
#include "moth_graphics/events/event_window.h"

#include <set>

class BoundsWidget;
class IEditorAction;
class EditorPanel;
class EditorApplication;

class EditorLayer : public moth_ui::Layer {
public:
    EditorLayer(moth_ui::Context& context, moth_graphics::graphics::IGraphics& graphics, EditorApplication* app);
    ~EditorLayer() override = default;

    moth_graphics::graphics::IGraphics& GetGraphics() const { return m_graphics; }

    bool OnEvent(moth_ui::Event const& event) override;

    void Update(uint32_t ticks) override;
    void Draw() override;
    void DebugDraw() override;

    void OnAddedToStack(moth_ui::LayerStack* layerStack) override;
    void OnRemovedFromStack() override;

    bool UseRenderSize() const override { return false; }

    void SetSelectedFrame(int frameNo);
    int GetSelectedFrame() const { return m_selectedFrame; }

    void ClearSelection();
    void AddSelection(std::shared_ptr<moth_ui::Node> node);
    void RemoveSelection(std::shared_ptr<moth_ui::Node> node);
    std::set<std::shared_ptr<moth_ui::Node>> const& GetSelection() const { return m_selection; }
    bool IsSelected(std::shared_ptr<moth_ui::Node> node) const;

    void LockNode(std::shared_ptr<moth_ui::Node> node);
    void UnlockNode(std::shared_ptr<moth_ui::Node> node);
    bool IsLocked(std::shared_ptr<moth_ui::Node> node) const;

    void Refresh();

    void BeginEditBounds(std::shared_ptr<moth_ui::Node> node = nullptr);
    void EndEditBounds();

    void BeginEditColor(std::shared_ptr<moth_ui::Node> node);
    void EndEditColor();

    void BeginEditRotation(std::shared_ptr<moth_ui::Node> node);
    void EndEditRotation();

    void PerformEditAction(std::unique_ptr<IEditorAction>&& editAction);
    void AddEditAction(std::unique_ptr<IEditorAction>&& editAction);
    int GetEditActionPos() const { return m_actionIndex; }

    std::vector<std::unique_ptr<IEditorAction>> const& GetEditActions() const { return m_editActions; }

    void NewLayout(bool discard = false);
    void LoadLayout(std::filesystem::path const& path, bool discard = false);

    std::shared_ptr<moth_ui::Group> GetRoot() const { return m_root; }
    std::shared_ptr<moth_ui::Layout> GetCurrentLayout() { return m_rootLayout; }
    std::filesystem::path const& GetCurrentLayoutPath() const { return m_currentLayoutPath; }

    template <typename T, typename... Args>
    T* AddEditorPanel(Args&&... args) {
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

    template <typename T>
    T* GetEditorPanel() const {
        auto const id = typeid(T).hash_code();
        auto const it = m_panels.find(id);
        if (std::end(m_panels) != it) {
            return static_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    template <typename T>
    void SetEditorPanelVisible(bool visible) {
        if (auto const panel = GetEditorPanel<T>()) {
            panel->m_visible = visible;
        }
    }

    template <typename T>
    bool IsEditorPanelFocused() const {
        if (auto const panel = GetEditorPanel<T>()) {
            return panel->IsFocused();
        }
        return false;
    }

    void ShowError(std::string const& message);

    ImGuiID GetDockID() const { return m_rootDockId; }

    void DeleteEntity();
    void ToggleEntityVisibility();

    EditorConfig& GetConfig() { return m_config; }
    moth_ui::Context& GetContext() const { return m_context; }

private:
    EditorApplication* m_app = nullptr;
    moth_ui::Context& m_context;
    moth_graphics::graphics::IGraphics& m_graphics;

    EditorConfig m_config;

    ImGuiID m_rootDockId = 0;

    std::map<size_t, std::unique_ptr<EditorPanel>> m_panels;

    std::filesystem::path m_currentLayoutPath;
    std::shared_ptr<moth_ui::Layout> m_rootLayout;
    std::shared_ptr<moth_ui::Group> m_root;
    std::set<std::shared_ptr<moth_ui::Node>> m_selection;
    std::set<std::shared_ptr<moth_ui::Node>> m_lockedNodes;
    std::vector<std::shared_ptr<moth_ui::LayoutEntity>> m_copiedEntities;

    int m_selectedFrame = 0;

    struct EditBoundsContext {
        std::shared_ptr<moth_ui::Node> node;
        std::shared_ptr<moth_ui::LayoutEntity> entity;
        moth_ui::LayoutRect originalRect;
        moth_ui::FloatVec2 originalPivot;
    };

    struct EditColorContext {
        std::shared_ptr<moth_ui::Node> node;
        std::shared_ptr<moth_ui::LayoutEntity> entity;
        moth_ui::Color originalColor;
    };

    struct EditRotationContext {
        std::shared_ptr<moth_ui::Node> node;
        std::shared_ptr<moth_ui::LayoutEntity> entity;
        float originalRotation = 0.0f;
    };

    std::vector<std::unique_ptr<EditBoundsContext>> m_editBoundsContext;
    std::unique_ptr<EditColorContext> m_editColorContext;
    std::unique_ptr<EditRotationContext> m_editRotationContext;
    std::vector<std::unique_ptr<IEditorAction>> m_editActions;
    int m_actionIndex = -1;
    int m_lastSaveActionIndex = -1;
    uint32_t m_autoSaveAccumulatedMs = 0;
    bool IsWorkPending() const { return m_lastSaveActionIndex != m_actionIndex; }
    ConfirmPrompt m_confirmPrompt;

    std::string m_lastErrorMsg;
    bool m_errorPending = false;

    static constexpr int MaxRecentFiles = 10;
    std::vector<std::filesystem::path> m_recentFiles;
    void AddRecentFile(std::filesystem::path const& path);

    void DrawMainMenu();

    void UndoEditAction();
    void RedoEditAction();
    void ClearEditActions();

    void SaveLayout(std::filesystem::path const& path);
    void AutoSave();
    void Rebuild();

    void MoveSelectionUp();
    void MoveSelectionDown();

    void MenuFuncNewLayout();
    void MenuFuncOpenLayout();
    void MenuFuncSaveLayout();
    void MenuFuncSaveLayoutAs();

    void CopyEntity();
    void CutEntity();
    void PasteEntity();

    void ResetCanvas();

    bool OnKey(moth_ui::EventKey const& event);
    bool OnRequestQuitEvent(moth_graphics::EventRequestQuit const& event);

    void Shutdown();
    void SaveConfig();
    void LoadConfig();

    std::unique_ptr<TexturePacker> m_texturePacker;
};
