#include "common.h"
#include "editor_layer.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/node.h"
#include "moth_ui/node_image.h"
#include "moth_ui/event_dispatch.h"
#include "animation_widget.h"
#include "moth_ui/animation_clip.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "editor/actions/add_action.h"
#include "editor/actions/delete_action.h"
#include "editor/actions/composite_action.h"
#include "editor/actions/modify_keyframe_action.h"
#include "editor/actions/add_keyframe_action.h"
#include "bounds_widget.h"
#include "properties_editor.h"
#include "moth_ui/utils/imgui_ext.h"
#include "layers/layer_stack.h"
#include "moth_ui/events/event_quit.h"
#include "preview_window.h"

EditorLayer::EditorLayer()
    : m_fileDialog(ImGuiFileBrowserFlags_EnterNewFilename)
    , m_boundsWidget(std::make_unique<BoundsWidget>(*this))
    , m_animationWidget(std::make_unique<AnimationWidget>(*this))
    , m_propertiesEditor(std::make_unique<PropertiesEditor>(*this))
    , m_previewWindow(std::make_unique<PreviewWindow>()) {
}

EditorLayer::~EditorLayer() {
}

std::unique_ptr<moth_ui::Event> EditorLayer::AlterMouseEvents(moth_ui::Event const& inEvent) {
    float const scaleFactor = 100.0f / m_displayZoom;
    if (auto const mouseDownEvent = moth_ui::event_cast<moth_ui::EventMouseDown>(inEvent)) {
        auto const position = static_cast<moth_ui::FloatVec2>(mouseDownEvent->GetPosition()) * scaleFactor;
        return std::make_unique<moth_ui::EventMouseDown>(mouseDownEvent->GetButton(), static_cast<moth_ui::IntVec2>(position));
    }
    if (auto const mouseUpEvent = moth_ui::event_cast<moth_ui::EventMouseUp>(inEvent)) {
        auto const position = static_cast<moth_ui::FloatVec2>(mouseUpEvent->GetPosition()) * scaleFactor;
        return std::make_unique<moth_ui::EventMouseUp>(mouseUpEvent->GetButton(), static_cast<moth_ui::IntVec2>(position));
    }
    if (auto const mouseMoveEvent = moth_ui::event_cast<moth_ui::EventMouseMove>(inEvent)) {
        auto const position = static_cast<moth_ui::FloatVec2>(mouseMoveEvent->GetPosition()) * scaleFactor;
        auto const delta = static_cast<moth_ui::FloatVec2>(mouseMoveEvent->GetDelta()) * scaleFactor;
        return std::make_unique<moth_ui::EventMouseMove>(static_cast<moth_ui::IntVec2>(position), delta);
    }
    return inEvent.Clone();
}

bool EditorLayer::OnEvent(moth_ui::Event const& event) {
    auto const alteredEvent = AlterMouseEvents(event);
    moth_ui::EventDispatch dispatch(*alteredEvent);
    dispatch.Dispatch(m_boundsWidget.get());
    dispatch.Dispatch(this, &EditorLayer::OnKey);
    dispatch.Dispatch(this, &EditorLayer::OnMouseDown);
    dispatch.Dispatch(this, &EditorLayer::OnMouseUp);
    dispatch.Dispatch(this, &EditorLayer::OnMouseMove);
    dispatch.Dispatch(this, &EditorLayer::OnMouseWheel);
    dispatch.Dispatch(m_previewWindow.get());
    return dispatch.GetHandled();
}

void EditorLayer::Update(uint32_t ticks) {
    m_previewWindow->Update(ticks);
}

void EditorLayer::Draw(SDL_Renderer& renderer) {
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    DrawCanvas(renderer);

    DrawMainMenu();
    DrawCanvasProperties();
    DrawPropertiesPanel();
    DrawAnimationPanel();
    DrawElementsPanel();
    DrawPreview();
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
            } else if (ImGui::MenuItem("Exit")) {
                m_layerStack->BroadcastEvent(moth_ui::EventQuit{});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Canvas Properties", &m_visibleCanvasProperties);
            ImGui::Checkbox("Properties", &m_visiblePropertiesPanel);
            ImGui::Checkbox("Animation", &m_visibleAnimationPanel);
            ImGui::Checkbox("Elements", &m_visibleElementsPanel);
            ImGui::Checkbox("Change Stack", &m_visibleUndoPanel);
            if (ImGui::Checkbox("Preview", &m_visiblePreview)) {
                if (m_visiblePreview) {
                    m_previewWindow->Refresh(m_rootLayout);
                }
            }
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
            } else if (ImGui::Button("Text")) {
                AddText();
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

void EditorLayer::DrawPreview() {
    if (m_visiblePreview) {
        if (ImGui::Begin("Preview", &m_visiblePreview)) {
            m_previewWindow->Draw();
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

    moth_ui::FloatVec2 const layerSize{ static_cast<float>(GetWidth()), static_cast<float>(GetHeight()) };
    moth_ui::FloatVec2 const displaySize{ static_cast<float>(m_displaySize.x), static_cast<float>(m_displaySize.y) };
    auto const preScaleSize = displaySize / scaleFactor;
    auto const preScaleOffset = m_canvasOffset + (layerSize - (displaySize / scaleFactor)) / 2.0f;

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
        moth_ui::IntRect displayRect;
        displayRect.topLeft = { guideRect.x, guideRect.y };
        displayRect.bottomRight = { guideRect.x + guideRect.w, guideRect.y + guideRect.h };
        m_root->SetScreenRect(displayRect);
        m_root->Draw();
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
        child->Refresh(m_selectedFrame);
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
    m_rootLayout = std::make_shared<moth_ui::LayoutEntityGroup>(moth_ui::LayoutRect{});
    m_selectedFrame = 0;
    m_editActions.clear();
    m_selection = nullptr;
    Rebuild();
}

void EditorLayer::LoadLayout(char const* path) {
    m_rootLayout = moth_ui::LoadLayout(path);
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
    auto newSubLayout = moth_ui::LoadLayout(path);
    moth_ui::LayoutRect bounds;
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
    moth_ui::LayoutRect bounds;
    bounds.anchor.topLeft = { 0.5f, 0.5f };
    bounds.anchor.bottomRight = { 0.5f, 0.5f };
    bounds.offset.topLeft = { -50, -50 };
    bounds.offset.bottomRight = { 50, 50 };

    auto newImageLayout = std::make_shared<moth_ui::LayoutEntityImage>(bounds);
    newImageLayout->m_texturePath = path;

    auto instance = newImageLayout->Instantiate();

    auto addAction = std::make_unique<AddAction>(std::move(instance), m_root);
    addAction->Do();
    AddEditAction(std::move(addAction));

    m_root->RecalculateBounds();
}

void EditorLayer::AddText() {
    moth_ui::LayoutRect bounds;
    bounds.anchor.topLeft = { 0.5f, 0.5f };
    bounds.anchor.bottomRight = { 0.5f, 0.5f };
    bounds.offset.topLeft = { -50, -50 };
    bounds.offset.bottomRight = { 50, 50 };

    auto newTextLayout = std::make_shared<moth_ui::LayoutEntityText>(bounds);
    auto instance = newTextLayout->Instantiate();
    auto addAction = std::make_unique<AddAction>(std::move(instance), m_root);
    addAction->Do();
    AddEditAction(std::move(addAction));

    m_root->RecalculateBounds();
}

void EditorLayer::Rebuild() {
    m_root = std::make_unique<moth_ui::Group>(m_rootLayout);
}

bool EditorLayer::OnKey(moth_ui::EventKey const& event) {
    if (event.GetAction() == moth_ui::KeyAction::Up) {
        switch (event.GetKey()) {
        case moth_ui::Key::Delete:
            if (m_selection) {
                auto delAction = std::make_unique<DeleteAction>(m_selection, m_root);
                delAction->Do();
                AddEditAction(std::move(delAction));
                SetSelection(nullptr);
            }
            return true;
        case moth_ui::Key::Z:
            UndoEditAction();
            return true;
        case moth_ui::Key::Y:
            RedoEditAction();
            return true;
        }
    }
    return false;
}

bool EditorLayer::OnMouseDown(moth_ui::EventMouseDown const& event) {
    if (event.GetButton() == moth_ui::MouseButton::Middle) {
        m_canvasGrabbed = true;
        return true;
    }
    return false;
}

bool EditorLayer::OnMouseUp(moth_ui::EventMouseUp const& event) {
    if (event.GetButton() == moth_ui::MouseButton::Middle) {
        m_canvasGrabbed = false;
    }
    return false;
}

bool EditorLayer::OnMouseMove(moth_ui::EventMouseMove const& event) {
    if (m_canvasGrabbed) {
        // undo the mouse scaling so if we're zoomed in moving isnt slow
        float const scaleFactor = 100.0f / m_displayZoom;
        m_canvasOffset += event.GetDelta() / scaleFactor;
    }
    return false;
}

bool EditorLayer::OnMouseWheel(moth_ui::EventMouseWheel const& event) {
    float const scaleFactor = 100.0f / m_displayZoom;
    m_displayZoom += static_cast<int>(event.GetDelta().y * 6 / scaleFactor);
    return false;
}

void EditorLayer::SetSelection(std::shared_ptr<moth_ui::Node> selection) {
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

    auto const SetTrackValue = [&](moth_ui::AnimationTrack::Target target, float value) {
        auto track = tracks.at(target);
        if (auto keyframePtr = track->GetKeyframe(frameNo)) {
            // keyframe exists
            auto const oldValue = keyframePtr->m_value;
            keyframePtr->m_value = value;
            editAction->GetActions().push_back(std::make_unique<ModifyKeyframeAction>(entity, target, frameNo, oldValue, value, keyframePtr->m_interpType, keyframePtr->m_interpType));
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
        SetTrackValue(moth_ui::AnimationTrack::Target::LeftAnchor, newRect.anchor.topLeft.x);
    }
    if (rectDelta.anchor.topLeft.y != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::TopAnchor, newRect.anchor.topLeft.y);
    }
    if (rectDelta.anchor.bottomRight.x != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::RightAnchor, newRect.anchor.bottomRight.x);
    }
    if (rectDelta.anchor.bottomRight.y != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::BottomAnchor, newRect.anchor.bottomRight.y);
    }
    if (rectDelta.offset.topLeft.x != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::LeftOffset, newRect.offset.topLeft.x);
    }
    if (rectDelta.offset.topLeft.y != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::TopOffset, newRect.offset.topLeft.y);
    }
    if (rectDelta.offset.bottomRight.x != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::RightOffset, newRect.offset.bottomRight.x);
    }
    if (rectDelta.offset.bottomRight.y != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::BottomOffset, newRect.offset.bottomRight.y);
    }

    if (!editAction->GetActions().empty()) {
        AddEditAction(std::move(editAction));
    }

    m_editBoundsContext.reset();
}

void EditorLayer::BeginEditColor() {
    if (!m_editColorContext && m_selection) {
        m_editColorContext = std::make_unique<EditColorContext>();
        m_editColorContext->entity = m_selection->GetLayoutEntity();
        m_editColorContext->originalColor = m_selection->GetColor();
    } else {
        assert(m_editColorContext->entity == m_selection->GetLayoutEntity());
    }
}

void EditorLayer::EndEditColor() {
    if (!m_editColorContext) {
        return;
    }
    auto entity = m_editColorContext->entity;
    auto& tracks = entity->GetAnimationTracks();
    int const frameNo = m_selectedFrame;
    std::unique_ptr<CompositeAction> editAction = std::make_unique<CompositeAction>();

    auto const SetTrackValue = [&](moth_ui::AnimationTrack::Target target, float value) {
        auto track = tracks.at(target);
        if (auto keyframePtr = track->GetKeyframe(frameNo)) {
            // keyframe exists
            auto const oldValue = keyframePtr->m_value;
            keyframePtr->m_value = value;
            editAction->GetActions().push_back(std::make_unique<ModifyKeyframeAction>(entity, target, frameNo, oldValue, value, keyframePtr->m_interpType, keyframePtr->m_interpType));
        } else {
            // no keyframe
            auto& keyframe = track->GetOrCreateKeyframe(frameNo);
            keyframe.m_value = value;
            editAction->GetActions().push_back(std::make_unique<AddKeyframeAction>(entity, target, frameNo, value));
        }
    };

    auto const& newColor = m_selection->GetColor();
    // dont use the color operators because we clamp values
    auto const colorDeltaR = newColor.r - m_editColorContext->originalColor.r;
    auto const colorDeltaG = newColor.g - m_editColorContext->originalColor.g;
    auto const colorDeltaB = newColor.b - m_editColorContext->originalColor.b;
    auto const colorDeltaA = newColor.a - m_editColorContext->originalColor.a;

    if (colorDeltaR != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::ColorRed, newColor.r);
    }
    if (colorDeltaG != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::ColorGreen, newColor.g);
    }
    if (colorDeltaB != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::ColorBlue, newColor.b);
    }
    if (colorDeltaA != 0) {
        SetTrackValue(moth_ui::AnimationTrack::Target::ColorAlpha, newColor.a);
    }

    if (!editAction->GetActions().empty()) {
        AddEditAction(std::move(editAction));
    }

    m_editColorContext.reset();
}