#include "common.h"
#include "editor_layer.h"
#include "uilib/layout/layout.h"
#include "uilib/node.h"
#include "uilib/node_image.h"
#include "uilib/event_dispatch.h"
#include "animation_widget.h"
#include "uilib/animation_clip.h"
#include "uilib/layout/layout_entity_image.h"
#include "editor/actions/add_action.h"
#include "editor/actions/delete_action.h"
#include "editor/actions/composite_action.h"
#include "editor/actions/modify_keyframe_action.h"
#include "editor/actions/add_keyframe_action.h"
#include "bounds_widget.h"
#include "properties_editor.h"
#include "uilib/utils/imgui_ext.h"

namespace ui {
    EditorLayer::EditorLayer()
        : m_fileDialog(ImGuiFileBrowserFlags_EnterNewFilename)
        , m_boundsWidget(std::make_unique<BoundsWidget>(*this))
        , m_animationWidget(std::make_unique<AnimationWidget>(*this))
        , m_propertiesEditor(std::make_unique<PropertiesEditor>(*this)) {
    }

    EditorLayer::~EditorLayer() {
    }

    std::unique_ptr<Event> EditorLayer::AlterMouseEvents(Event const& inEvent) {
        float const scaleFactor = 100.0f / m_displayZoom;
        if (auto const mouseDownEvent = event_cast<EventMouseDown>(inEvent)) {
            auto const position = mouseDownEvent->GetPosition() * scaleFactor;
            return std::make_unique<EventMouseDown>(mouseDownEvent->GetButton(), position);
        }
        if (auto const mouseUpEvent = event_cast<EventMouseUp>(inEvent)) {
            auto const position = mouseUpEvent->GetPosition() * scaleFactor;
            return std::make_unique<EventMouseUp>(mouseUpEvent->GetButton(), position);
        }
        if (auto const mouseMoveEvent = event_cast<EventMouseMove>(inEvent)) {
            auto const position = mouseMoveEvent->GetPosition() * scaleFactor;
            auto const delta = mouseMoveEvent->GetDelta() * scaleFactor;
            return std::make_unique<EventMouseMove>(position, delta);
        }
        return inEvent.Clone();
    }

    bool EditorLayer::OnEvent(Event const& event) {
        auto const alteredEvent = AlterMouseEvents(event);
        EventDispatch dispatch(*alteredEvent);
        dispatch.Dispatch(m_boundsWidget.get());
        dispatch.Dispatch(this, &EditorLayer::OnKey);
        dispatch.Dispatch(this, &EditorLayer::OnMouseDown);
        dispatch.Dispatch(this, &EditorLayer::OnMouseUp);
        dispatch.Dispatch(this, &EditorLayer::OnMouseMove);
        dispatch.Dispatch(this, &EditorLayer::OnMouseWheel);
        return dispatch.GetHandled();
    }

    void EditorLayer::Update(uint32_t ticks) {
    }

    void EditorLayer::Draw(SDL_Renderer& renderer) {
        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        DrawCanvas(renderer);

        DrawMainMenu();
        DrawCanvasProperties();
        DrawPropertiesPanel();
        DrawAnimationPanel();
        DrawElementsPanel();
        DrawUndoStack();

        m_fileDialog.Display();
        if (m_fileDialog.HasSelected()) {
            if (m_fileOpenMode == FileOpenMode::Layout) {
                LoadLayout(m_fileDialog.GetSelected().string().c_str());
                m_fileDialog.ClearSelected();
            } else if (m_fileOpenMode == FileOpenMode::SubLayout) {
                AddSubLayout(m_fileDialog.GetSelected().string().c_str());
                m_fileDialog.ClearSelected();
            } else if (m_fileOpenMode == FileOpenMode::Image) {
                AddImage(m_fileDialog.GetSelected().string().c_str());
                m_fileDialog.ClearSelected();
            } else if (m_fileOpenMode == FileOpenMode::Save) {
                SaveLayout(m_fileDialog.GetSelected().string().c_str());
                m_fileDialog.ClearSelected();
            }
        }
    }

    void EditorLayer::DrawMainMenu() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) {
                    NewLayout();
                } else if (ImGui::MenuItem("Open..", "Ctrl+O")) {
                    m_fileDialog.SetTitle("Open..");
                    m_fileDialog.SetTypeFilters({ ".json" });
                    m_fileDialog.Open();
                    m_fileOpenMode = FileOpenMode::Layout;
                } else if (ImGui::MenuItem("Save..", "Ctrl+S")) {
                    m_fileDialog.SetTitle("Save..");
                    m_fileDialog.SetTypeFilters({ ".json" });
                    m_fileDialog.Open();
                    m_fileOpenMode = FileOpenMode::Save;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                ImGui::Checkbox("Canvas Properties", &m_visibleCanvasProperties);
                ImGui::Checkbox("Properties", &m_visiblePropertiesPanel);
                ImGui::Checkbox("Animation", &m_visibleAnimationPanel);
                ImGui::Checkbox("Elements", &m_visibleElementsPanel);
                ImGui::Checkbox("Change Stack", &m_visibleUndoPanel);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void EditorLayer::DrawCanvasProperties() {
        if (m_visibleCanvasProperties) {
            if (ImGui::Begin("Canvas Properties", &m_visibleCanvasProperties)) {
                imgui_ext::InputIntVec2("Display Size", &m_displaySize);
                ImGui::InputInt("Display Zoom", &m_displayZoom);
                m_displayZoom = std::clamp(m_displayZoom, s_minZoom, s_maxZoom);
                imgui_ext::InputFloatVec2("Display Offset", &m_canvasOffset);
                ImGui::InputInt("Grid Spacing", &m_gridSpacing);
                m_gridSpacing = std::clamp(m_gridSpacing, 0, m_displaySize.x / 2);
            }
            ImGui::End();
        }
    }

    void EditorLayer::DrawPropertiesPanel() {
        if (m_visiblePropertiesPanel) {
            if (ImGui::Begin("Properties", &m_visiblePropertiesPanel)) {
                m_propertiesEditor->Draw();
            }
            ImGui::End();
        }
    }

    void EditorLayer::DrawElementsPanel() {
        if (m_visibleElementsPanel) {
            if (ImGui::Begin("Elements", &m_visibleElementsPanel)) {
                if (ImGui::Button("Image")) {
                    m_fileDialog.SetTitle("Open..");
                    m_fileDialog.SetTypeFilters({ ".jpg", ".jpeg", ".png", ".bmp" });
                    m_fileDialog.Open();
                    m_fileOpenMode = FileOpenMode::Image;
                } else if (ImGui::Button("SubLayout")) {
                    m_fileDialog.SetTitle("Open..");
                    m_fileDialog.SetTypeFilters({ ".json" });
                    m_fileDialog.Open();
                    m_fileOpenMode = FileOpenMode::SubLayout;
                }
            }
            ImGui::End();
        }
    }

    void EditorLayer::DrawAnimationPanel() {
        if (m_visibleAnimationPanel) {
            if (ImGui::Begin("Animation", &m_visibleAnimationPanel)) {
                m_animationWidget->Draw();
            }
            ImGui::End();
        }
    }

    void EditorLayer::DrawUndoStack() {
        if (m_visibleUndoPanel) {
            if (ImGui::Begin("Undo Stack", &m_visibleUndoPanel)) {
                int i = 0;
                for (auto&& edit : m_editActions) {
                    ImGui::PushID(edit.get());
                    if (i == m_actionIndex) {
                        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    }
                    edit->OnImGui();
                    if (i == m_actionIndex) {
                        ImGui::PopStyleColor();
                    }
                    ImGui::PopID();
                    ++i;
                }
            }
            ImGui::End();
        }
    }

    void EditorLayer::DrawCanvas(SDL_Renderer& renderer) {
        SDL_SetRenderDrawColor(&renderer, 0xAA, 0xAA, 0xAA, 0xFF);
        SDL_RenderClear(&renderer);

        float const scaleFactor = 100.0f / m_displayZoom;

        // first draw the canvas and the grid lines before scaling so  they stay at fine resolution

        FloatVec2 const layerSize{ static_cast<float>(GetWidth()), static_cast<float>(GetHeight()) };
        FloatVec2 const displaySize{ static_cast<float>(m_displaySize.x), static_cast<float>(m_displaySize.y) };
        FloatVec2 const preScaleSize = displaySize / scaleFactor;
        FloatVec2 const preScaleOffset = m_canvasOffset + (layerSize - (displaySize / scaleFactor)) / 2.0f;

        SDL_FRect canvasRect{ preScaleOffset.x,
                              preScaleOffset.y,
                              preScaleSize.x,
                              preScaleSize.y };

        // canvas
        SDL_SetRenderDrawColor(&renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        SDL_RenderFillRectF(&renderer, &canvasRect);

        // grid lines
        if (m_gridSpacing > 0) {
            int const vertGridCount = (m_displaySize.x - 1) / m_gridSpacing;
            int const horizGridCount = (m_displaySize.y - 1) / m_gridSpacing;
            int index = 0;
            float gridX = m_gridSpacing / scaleFactor;
            SDL_SetRenderDrawColor(&renderer, 0xDD, 0xDD, 0xDD, 0xFF);
            for (int i = 0; i < vertGridCount; ++i) {
                int const x = static_cast<int>(gridX);
                SDL_RenderDrawLineF(&renderer, canvasRect.x + x, canvasRect.y, canvasRect.x + x, canvasRect.y + canvasRect.h - 1);
                gridX += m_gridSpacing / scaleFactor;
            }
            float gridY = m_gridSpacing / scaleFactor;
            for (int i = 0; i < horizGridCount; ++i) {
                int const y = static_cast<int>(gridY);
                SDL_RenderDrawLineF(&renderer, canvasRect.x, canvasRect.y + y, canvasRect.x + canvasRect.w - 1, canvasRect.y + y);
                gridY += m_gridSpacing / scaleFactor;
            }
        }

        // setup scaling and draw the layout
        int oldRenderWidth, oldRenderHeight;
        SDL_RenderGetLogicalSize(&renderer, &oldRenderWidth, &oldRenderHeight);
        int const newRenderWidth = static_cast<int>(oldRenderWidth * scaleFactor);
        int const newRenderHeight = static_cast<int>(oldRenderHeight * scaleFactor);
        int const newRenderOffsetX = static_cast<int>(m_canvasOffset.x * scaleFactor);
        int const newRenderOffsetY = static_cast<int>(m_canvasOffset.y * scaleFactor);
        SDL_RenderSetLogicalSize(&renderer, newRenderWidth, newRenderHeight);
        SDL_Rect guideRect{ newRenderOffsetX + (newRenderWidth - m_displaySize.x) / 2, newRenderOffsetY + (newRenderHeight - m_displaySize.y) / 2, m_displaySize.x, m_displaySize.y };
        m_canvasTopLeft = { guideRect.x, guideRect.y };

        if (m_root) {
            IntRect displayRect;
            displayRect.topLeft = { guideRect.x, guideRect.y };
            displayRect.bottomRight = { guideRect.x + guideRect.w, guideRect.y + guideRect.h };
            m_root->SetScreenRect(displayRect);
            m_root->Draw(renderer);
        }
        SDL_RenderSetLogicalSize(&renderer, oldRenderWidth, oldRenderHeight);

        m_boundsWidget->Draw(renderer); // TODO we want this non scaled
    }

    void EditorLayer::DebugDraw() {
    }

    void EditorLayer::OnAddedToStack(LayerStack* layerStack) {
        Layer::OnAddedToStack(layerStack);
        NewLayout();
    }

    void EditorLayer::OnRemovedFromStack() {
        Layer::OnRemovedFromStack();
    }

    void EditorLayer::AddEditAction(std::unique_ptr<IEditorAction>&& editAction) {
        // discard anything past the current action
        while ((static_cast<int>(m_editActions.size()) - 1) > m_actionIndex) {
            m_editActions.pop_back();
        }
        m_editActions.push_back(std::move(editAction));
        ++m_actionIndex;
    }

    void EditorLayer::SetSelectedFrame(int frameNo) {
        if (m_selectedFrame != frameNo) {
            m_selectedFrame = frameNo;
            Refresh();
        }
    }

    void EditorLayer::Refresh() {
        for (auto&& child : m_root->GetChildren()) {
            child->RefreshBounds(m_selectedFrame);
        }
    }

    void EditorLayer::UndoEditAction() {
        if (m_actionIndex >= 0) {
            m_editActions[m_actionIndex]->Undo();
            --m_actionIndex;
            Refresh();
            m_animationWidget->OnUndo();
        }
    }

    void EditorLayer::RedoEditAction() {
        if (m_actionIndex < (static_cast<int>(m_editActions.size()) - 1)) {
            ++m_actionIndex;
            m_editActions[m_actionIndex]->Do();
            Refresh();
            m_animationWidget->OnRedo();
        }
    }

    void EditorLayer::ClearEditActions() {
        m_editActions.clear();
        m_actionIndex = -1;
    }

    void EditorLayer::NewLayout() {
        m_rootLayout = std::make_shared<LayoutEntityGroup>(LayoutRect{});
        m_selectedFrame = 0;
        m_editActions.clear();
        m_selection = nullptr;
        Rebuild();
    }

    void EditorLayer::LoadLayout(char const* path) {
        m_rootLayout = ui::LoadLayout(path);
        m_selectedFrame = 0;
        m_editActions.clear();
        m_selection = nullptr;
        Rebuild();
    }

    void EditorLayer::SaveLayout(char const* path) {
        std::ofstream ofile(path);
        if (!ofile.is_open()) {
            return;
        }

        nlohmann::json json = m_rootLayout->Serialize();
        ofile << json;
    }

    void EditorLayer::AddSubLayout(char const* path) {
        auto newSubLayout = ui::LoadLayout(path);
        LayoutRect bounds;
        bounds.anchor.topLeft = { 0.5f, 0.5f };
        bounds.anchor.bottomRight = { 0.5f, 0.5f };
        bounds.offset.topLeft = { -50, -50 };
        bounds.offset.bottomRight = { 50, 50 };
        newSubLayout->SetBounds(bounds, 0);

        auto instance = newSubLayout->Instantiate();

        auto addAction = std::make_unique<AddAction>(std::move(instance), m_root);
        addAction->Do();
        AddEditAction(std::move(addAction));

        m_root->RecalculateBounds();
    }

    void EditorLayer::AddImage(char const* path) {
        LayoutRect bounds;
        bounds.anchor.topLeft = { 0.5f, 0.5f };
        bounds.anchor.bottomRight = { 0.5f, 0.5f };
        bounds.offset.topLeft = { -50, -50 };
        bounds.offset.bottomRight = { 50, 50 };

        auto newImageLayout = std::make_shared<LayoutEntityImage>(bounds);
        newImageLayout->m_texturePath = path;

        auto instance = newImageLayout->Instantiate();

        auto addAction = std::make_unique<AddAction>(std::move(instance), m_root);
        addAction->Do();
        AddEditAction(std::move(addAction));

        m_root->RecalculateBounds();
    }

    void EditorLayer::Rebuild() {
        m_root = std::make_unique<Group>(m_rootLayout);
    }

    bool EditorLayer::OnKey(EventKey const& event) {
        if (event.GetAction() == KeyAction::Up) {
            switch (event.GetKey()) {
            case Key::Delete:
                if (m_selection) {
                    auto delAction = std::make_unique<DeleteAction>(m_selection, m_root);
                    delAction->Do();
                    AddEditAction(std::move(delAction));
                    SetSelection(nullptr);
                }
                return true;
            case Key::Z:
                UndoEditAction();
                return true;
            case Key::Y:
                RedoEditAction();
                return true;
            }
        }
        return false;
    }

    bool EditorLayer::OnMouseDown(EventMouseDown const& event) {
        if (event.GetButton() == MouseButton::Middle) {
            m_canvasGrabbed = true;
            return true;
        }
        return false;
    }

    bool EditorLayer::OnMouseUp(EventMouseUp const& event) {
        if (event.GetButton() == MouseButton::Middle) {
            m_canvasGrabbed = false;
        }
        return false;
    }

    bool EditorLayer::OnMouseMove(EventMouseMove const& event) {
        if (m_canvasGrabbed) {
            // undo the mouse scaling so if we're zoomed in moving isnt slow
            float const scaleFactor = 100.0f / m_displayZoom;
            m_canvasOffset += event.GetDelta() / scaleFactor;
        }
        return false;
    }

    bool EditorLayer::OnMouseWheel(EventMouseWheel const& event) {
        float const scaleFactor = 100.0f / m_displayZoom;
        m_displayZoom += static_cast<int>(event.GetDelta().y * 6 / scaleFactor);
        return false;
    }

    void EditorLayer::SetSelection(std::shared_ptr<Node> selection) {
        if (selection != m_selection) {
            if (m_editBoundsContext) {
                EndEditBounds();
            }
        }
        m_selection = selection;
    }

    void EditorLayer::BeginEditBounds() {
        if (!m_editBoundsContext && m_selection) {
            m_editBoundsContext = std::make_unique<EditBoundsContext>();
            m_editBoundsContext->entity = m_selection->GetLayoutEntity();
            m_editBoundsContext->originalRect = m_selection->GetLayoutRect();
        } else {
            assert(m_editBoundsContext->entity == m_selection->GetLayoutEntity());
        }
    }

    void EditorLayer::EndEditBounds() {
        if (!m_editBoundsContext) {
            return;
        }
        auto entity = m_editBoundsContext->entity;
        auto& tracks = entity->GetAnimationTracks();
        int const frameNo = m_selectedFrame;
        std::unique_ptr<CompositeAction> editAction = std::make_unique<CompositeAction>();

        auto const SetTrackValue = [&](AnimationTrack::Target target, float value) {
            auto track = tracks.at(target);
            if (auto keyframePtr = track->GetKeyframe(frameNo)) {
                // keyframe exists
                auto const oldValue = keyframePtr->m_value;
                keyframePtr->m_value = value;
                editAction->GetActions().push_back(std::make_unique<ModifyKeyframeAction>(entity, target, frameNo, oldValue, value));
            } else {
                // no keyframe
                auto& keyframe = track->GetOrCreateKeyframe(frameNo);
                keyframe.m_value = value;
                editAction->GetActions().push_back(std::make_unique<AddKeyframeAction>(entity, target, frameNo, value));
            }
        };

        auto const& newRect = m_selection->GetLayoutRect();
        auto const rectDelta = newRect - m_editBoundsContext->originalRect;

        if (rectDelta.anchor.topLeft.x != 0) {
            SetTrackValue(AnimationTrack::Target::LeftAnchor, newRect.anchor.topLeft.x);
        }
        if (rectDelta.anchor.topLeft.y != 0) {
            SetTrackValue(AnimationTrack::Target::TopAnchor, newRect.anchor.topLeft.y);
        }
        if (rectDelta.anchor.bottomRight.x != 0) {
            SetTrackValue(AnimationTrack::Target::RightAnchor, newRect.anchor.bottomRight.x);
        }
        if (rectDelta.anchor.bottomRight.y != 0) {
            SetTrackValue(AnimationTrack::Target::BottomAnchor, newRect.anchor.bottomRight.y);
        }
        if (rectDelta.offset.topLeft.x != 0) {
            SetTrackValue(AnimationTrack::Target::LeftOffset, newRect.offset.topLeft.x);
        }
        if (rectDelta.offset.topLeft.y != 0) {
            SetTrackValue(AnimationTrack::Target::TopOffset, newRect.offset.topLeft.y);
        }
        if (rectDelta.offset.bottomRight.x != 0) {
            SetTrackValue(AnimationTrack::Target::RightOffset, newRect.offset.bottomRight.x);
        }
        if (rectDelta.offset.bottomRight.y != 0) {
            SetTrackValue(AnimationTrack::Target::BottomOffset, newRect.offset.bottomRight.y);
        }

        if (!editAction->GetActions().empty()) {
            AddEditAction(std::move(editAction));
        }

        m_editBoundsContext.reset();
    }
}
