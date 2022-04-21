#include "common.h"
#include "editor_layer.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/node.h"
#include "moth_ui/node_image.h"
#include "moth_ui/event_dispatch.h"
#include "moth_ui/animation_clip.h"
#include "moth_ui/layout/layout_entity_ref.h"
#include "moth_ui/layout/layout_entity_rect.h"
#include "moth_ui/layout/layout_entity_image.h"
#include "moth_ui/layout/layout_entity_text.h"
#include "moth_ui/layout/layout_entity_clip.h"
#include "editor/actions/add_action.h"
#include "editor/actions/delete_action.h"
#include "editor/actions/composite_action.h"
#include "editor/actions/modify_keyframe_action.h"
#include "editor/actions/add_keyframe_action.h"
#include "editor/actions/change_index_action.h"
#include "bounds_widget.h"
#include "moth_ui/utils/imgui_ext.h"
#include "layers/layer_stack.h"
#include "events/event.h"
#include "app.h"
#include "imgui_internal.h"
#include "panels/editor_panel_layout_list.h"
#include "panels/editor_panel_image_list.h"
#include "panels/editor_panel_canvas_properties.h"
#include "panels/editor_panel_project_properties.h"
#include "panels/editor_panel_properties.h"
#include "panels/editor_panel_elements.h"
#include "panels/editor_panel_animation.h"
#include "panels/editor_panel_keyframes.h"
#include "panels/editor_panel_undo_stack.h"
#include "panels/editor_panel_preview.h"

extern App* g_App;

namespace {
    enum class FileOpenMode {
        Unknown,
        OpenLayout,
        SaveLayout,
        OpenProject,
        SaveProject,
    };
    static FileOpenMode s_fileOpenMode = FileOpenMode::Unknown;
    static ImGui::FileBrowser s_fileDialog(ImGuiFileBrowserFlags_EnterNewFilename);
}

EditorLayer::EditorLayer()
    : m_boundsWidget(std::make_unique<BoundsWidget>(*this)) {
    m_layoutProject.m_layoutRoot = ".";
    m_layoutProject.m_imageRoot = ".";

    AddEditorPanel<EditorPanelCanvasProperties>(*this, true);
    AddEditorPanel<EditorPanelProjectProperties>(*this, false);
    AddEditorPanel<EditorPanelLayoutList>(*this, true);
    AddEditorPanel<EditorPanelImageList>(*this, true);
    AddEditorPanel<EditorPanelProperties>(*this, true);
    AddEditorPanel<EditorPanelElements>(*this, true);
    auto const animationPanel = AddEditorPanel<EditorPanelAnimation>(*this, true);
    AddEditorPanel<EditorPanelKeyframes>(*this, *animationPanel);
    AddEditorPanel<EditorPanelUndoStack>(*this, false);
    AddEditorPanel<EditorPanelPreview>(*this, false);

    for (auto& [type, panel] : m_panels) {
        panel->Refresh();
    }
}

std::unique_ptr<moth_ui::Event> EditorLayer::AlterMouseEvents(moth_ui::Event const& inEvent) {
    float const scaleFactor = 100.0f / m_canvasProperties.m_zoom;
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
    dispatch.Dispatch(this, &EditorLayer::OnRequestQuitEvent);
    for (auto& [type, panel] : m_panels) {
        dispatch.Dispatch(panel.get());
    }
    return dispatch.GetHandled();
}

void EditorLayer::Update(uint32_t ticks) {
    for (auto& [type, panel] : m_panels) {
        panel->Update(ticks);
    }

    auto const windowTitle = fmt::format("{}{}", m_currentLayoutPath.empty() ? "New Layout" : m_currentLayoutPath, IsWorkPending() ? " *" : "");
    g_App->SetWindowTitle(windowTitle);
}

void EditorLayer::Draw(SDL_Renderer& renderer) {
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    m_confirmPrompt.Draw();

    DrawCanvas(renderer);
    DrawMainMenu();

    for (auto& [type, panel] : m_panels) {
        panel->Draw();
    }

    s_fileDialog.Display();
    if (s_fileDialog.HasSelected()) {
        if (s_fileOpenMode == FileOpenMode::OpenProject) {
            LoadProject(s_fileDialog.GetSelected().string().c_str());
            s_fileDialog.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::SaveProject) {
            SaveProject(s_fileDialog.GetSelected().string().c_str());
            s_fileDialog.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::OpenLayout) {
            LoadLayout(s_fileDialog.GetSelected().string().c_str());
            s_fileDialog.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::SaveLayout) {
            SaveLayout(s_fileDialog.GetSelected().string().c_str());
            s_fileDialog.ClearSelected();
        }
    }
}

void EditorLayer::DrawMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Layout")) {
                NewLayout();
            } 
            if (ImGui::MenuItem("Open Layout")) {
                s_fileDialog.SetTitle("Open Layout..");
                s_fileDialog.SetTypeFilters({ ".json" });
                s_fileDialog.Open();
                s_fileOpenMode = FileOpenMode::OpenLayout;
            }
            if (ImGui::MenuItem("Save Layout", nullptr, nullptr, !m_currentLayoutPath.empty())) {
                SaveLayout(m_currentLayoutPath.c_str());
            }
            if (ImGui::MenuItem("Save Layout As")) {
                s_fileDialog.SetTitle("Save Layout As..");
                s_fileDialog.SetTypeFilters({ ".json" });
                s_fileDialog.Open();
                s_fileOpenMode = FileOpenMode::SaveLayout;
            }
            if (ImGui::MenuItem("Open Project..")) {
                s_fileDialog.SetTitle("Open Project..");
                s_fileDialog.SetTypeFilters({ ".json" });
                s_fileDialog.Open();
                s_fileOpenMode = FileOpenMode::OpenProject;
            }
            if (ImGui::MenuItem("Save Project", nullptr, nullptr, !m_layoutProject.m_loadedPath.empty())) {
                SaveProject(m_layoutProject.m_loadedPath.c_str());
            }
            if (ImGui::MenuItem("Save Project As..")) {
                s_fileDialog.SetTitle("Save Project..");
                s_fileDialog.SetTypeFilters({ ".json" });
                s_fileDialog.Open();
                s_fileOpenMode = FileOpenMode::SaveProject;
            }
            if (ImGui::MenuItem("Exit")) {
                m_layerStack->BroadcastEvent(EventQuit{});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            std::map<std::string, bool*> sortedVisBools;
            for (auto& [type, panel] : m_panels) {
                if (panel->IsExposed()) {
                    sortedVisBools.insert(std::make_pair(panel->GetTitle(), &panel->m_visible));
                }
            }
            for (auto&& [title, visible] : sortedVisBools) {
                ImGui::Checkbox(title.c_str(), visible);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EditorLayer::DrawCanvas(SDL_Renderer& renderer) {
    SDL_SetRenderDrawColor(&renderer, 0xAA, 0xAA, 0xAA, 0xFF);
    SDL_RenderClear(&renderer);

    float const scaleFactor = 100.0f / m_canvasProperties.m_zoom;

    // first draw the canvas and the grid lines before scaling so  they stay at fine resolution

    moth_ui::FloatVec2 const layerSize{ static_cast<float>(GetWidth()), static_cast<float>(GetHeight()) };
    moth_ui::FloatVec2 const displaySize{ static_cast<float>(m_canvasProperties.m_size.x), static_cast<float>(m_canvasProperties.m_size.y) };
    auto const preScaleSize = displaySize / scaleFactor;
    auto const preScaleOffset = m_canvasProperties.m_offset + (layerSize - (displaySize / scaleFactor)) / 2.0f;

    SDL_FRect canvasRect{ preScaleOffset.x,
                          preScaleOffset.y,
                          preScaleSize.x,
                          preScaleSize.y };

    // canvas
    SDL_SetRenderDrawColor(&renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderFillRectF(&renderer, &canvasRect);

    // grid lines
    if (m_canvasProperties.m_gridSpacing > 0) {
        int const vertGridCount = (m_canvasProperties.m_size.x - 1) / m_canvasProperties.m_gridSpacing;
        int const horizGridCount = (m_canvasProperties.m_size.y - 1) / m_canvasProperties.m_gridSpacing;
        int index = 0;
        float gridX = m_canvasProperties.m_gridSpacing / scaleFactor;
        SDL_SetRenderDrawColor(&renderer, 0xDD, 0xDD, 0xDD, 0xFF);
        for (int i = 0; i < vertGridCount; ++i) {
            int const x = static_cast<int>(gridX);
            SDL_RenderDrawLineF(&renderer, canvasRect.x + x, canvasRect.y, canvasRect.x + x, canvasRect.y + canvasRect.h - 1);
            gridX += m_canvasProperties.m_gridSpacing / scaleFactor;
        }
        float gridY = m_canvasProperties.m_gridSpacing / scaleFactor;
        for (int i = 0; i < horizGridCount; ++i) {
            int const y = static_cast<int>(gridY);
            SDL_RenderDrawLineF(&renderer, canvasRect.x, canvasRect.y + y, canvasRect.x + canvasRect.w - 1, canvasRect.y + y);
            gridY += m_canvasProperties.m_gridSpacing / scaleFactor;
        }
    }

    // setup scaling and draw the layout
    int oldRenderWidth, oldRenderHeight;
    SDL_RenderGetLogicalSize(&renderer, &oldRenderWidth, &oldRenderHeight);
    int const newRenderWidth = static_cast<int>(oldRenderWidth * scaleFactor);
    int const newRenderHeight = static_cast<int>(oldRenderHeight * scaleFactor);
    int const newRenderOffsetX = static_cast<int>(m_canvasProperties.m_offset.x * scaleFactor);
    int const newRenderOffsetY = static_cast<int>(m_canvasProperties.m_offset.y * scaleFactor);
    SDL_RenderSetLogicalSize(&renderer, newRenderWidth, newRenderHeight);
    SDL_Rect guideRect{ newRenderOffsetX + (newRenderWidth - m_canvasProperties.m_size.x) / 2, newRenderOffsetY + (newRenderHeight - m_canvasProperties.m_size.y) / 2, m_canvasProperties.m_size.x, m_canvasProperties.m_size.y };
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

void EditorLayer::PerformEditAction(std::unique_ptr<IEditorAction>&& editAction) {
    editAction->Do();
    AddEditAction(std::move(editAction));
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
    }
}

void EditorLayer::RedoEditAction() {
    if (m_actionIndex < (static_cast<int>(m_editActions.size()) - 1)) {
        ++m_actionIndex;
        m_editActions[m_actionIndex]->Do();
        Refresh();
    }
}

void EditorLayer::ClearEditActions() {
    m_editActions.clear();
    m_actionIndex = -1;
    m_actionIndex = -1;
    m_lastSaveActionIndex = -1;
}

void EditorLayer::NewLayout(bool discard) {
    if (!discard && IsWorkPending()) {
        m_confirmPrompt.SetTitle("Save?");
        m_confirmPrompt.SetMessage("You have unsaved work? Save?");
        m_confirmPrompt.SetPositiveText("Save");
        m_confirmPrompt.SetNegativeText("Discard");
        m_confirmPrompt.SetPositiveAction([this]() {
            SaveLayout(m_currentLayoutPath.c_str());
            NewLayout();
        });
        m_confirmPrompt.SetNegativeAction([this]() {
            NewLayout(true);
        });
        m_confirmPrompt.Open();
    } else {
        m_rootLayout = std::make_shared<moth_ui::Layout>();
        m_selectedFrame = 0;
        m_selection = nullptr;
        ClearEditActions();
        Rebuild();
    }
}

void EditorLayer::LoadProject(char const* path) {
    std::ifstream ifile(path);
    if (!ifile.is_open()) {
        return;
    }

    nlohmann::json json;
    ifile >> json;

    m_layoutProject = json;

    std::filesystem::path projectPath(path);
    std::filesystem::path layoutPath = projectPath.parent_path() / m_layoutProject.m_layoutRoot;
    std::filesystem::path imagePath = projectPath.parent_path() / m_layoutProject.m_imageRoot;

    m_layoutProject.m_layoutRoot = layoutPath.string();
    m_layoutProject.m_imageRoot = imagePath.string();
    m_layoutProject.m_loadedPath = projectPath.string();

    GetEditorPanel<EditorPanelLayoutList>()->Refresh();
    GetEditorPanel<EditorPanelImageList>()->Refresh();

    NewLayout();
}

void EditorLayer::SaveProject(char const* path) {
    std::ofstream ofile(path);
    if (!ofile.is_open()) {
        return;
    }

    // only save out relative paths to the project file
    std::filesystem::path projectPath(path);
    std::filesystem::path layoutsPath(m_layoutProject.m_layoutRoot);
    std::filesystem::path imagesPath(m_layoutProject.m_imageRoot);

    LayoutProject projectCopy = m_layoutProject;
    projectCopy.m_layoutRoot = std::filesystem::relative(projectCopy.m_layoutRoot, projectPath.parent_path()).string();
    projectCopy.m_imageRoot = std::filesystem::relative(projectCopy.m_imageRoot, projectPath.parent_path()).string();
    nlohmann::json json = projectCopy;
    ofile << json;
}

void EditorLayer::LoadLayout(char const* path, bool discard) {
    if (!discard && IsWorkPending()) {
        std::string const pathStr = path;
        m_confirmPrompt.SetTitle("Save?");
        m_confirmPrompt.SetMessage("You have unsaved work? Save?");
        m_confirmPrompt.SetPositiveText("Save");
        m_confirmPrompt.SetNegativeText("Discard");
        m_confirmPrompt.SetPositiveAction([this, pathStr]() {
            SaveLayout(m_currentLayoutPath.c_str());
            LoadLayout(pathStr.c_str());
        });
        m_confirmPrompt.SetNegativeAction([this, pathStr]() {
            LoadLayout(pathStr.c_str(), true);
        });
        m_confirmPrompt.Open();
    } else {
        m_rootLayout = moth_ui::Layout::Load(path);
        m_currentLayoutPath = path;
        m_selectedFrame = 0;
        m_selection = nullptr;
        ClearEditActions();
        Rebuild();
    }
}

void EditorLayer::SaveLayout(char const* path) {
    if (m_rootLayout->Save(path)) {
        m_lastSaveActionIndex = m_actionIndex;
        m_currentLayoutPath = path;
    }
}

void EditorLayer::AddSubLayout(char const* path) {
    auto newSubLayout = std::make_shared<moth_ui::LayoutEntityRef>(path, *moth_ui::Layout::Load(path));
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
    newImageLayout->m_imagePath = path;

    auto instance = newImageLayout->Instantiate();

    auto addAction = std::make_unique<AddAction>(std::move(instance), m_root);
    addAction->Do();
    AddEditAction(std::move(addAction));

    m_root->RecalculateBounds();
}

void EditorLayer::AddRect() {
    moth_ui::LayoutRect bounds;
    bounds.anchor.topLeft = { 0.5f, 0.5f };
    bounds.anchor.bottomRight = { 0.5f, 0.5f };
    bounds.offset.topLeft = { -50, -50 };
    bounds.offset.bottomRight = { 50, 50 };

    auto newLayoutEntity = std::make_shared<moth_ui::LayoutEntityRect>(bounds);
    auto instance = newLayoutEntity->Instantiate();
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

void EditorLayer::AddClip() {
    moth_ui::LayoutRect bounds;
    bounds.anchor.topLeft = { 0.5f, 0.5f };
    bounds.anchor.bottomRight = { 0.5f, 0.5f };
    bounds.offset.topLeft = { -50, -50 };
    bounds.offset.bottomRight = { 50, 50 };

    auto newLayout = std::make_shared<moth_ui::LayoutEntityClip>(bounds);
    auto instance = newLayout->Instantiate();
    auto addAction = std::make_unique<AddAction>(std::move(instance), m_root);
    PerformEditAction(std::move(addAction));

    m_root->RecalculateBounds();
}

void EditorLayer::Rebuild() {
    m_root = std::make_unique<moth_ui::Group>(m_rootLayout);
}

void EditorLayer::MoveSelectionUp() {
    if (!m_selection || !m_selection->GetParent()) {
        return;
    }

    auto parent = m_selection->GetParent();
    auto& children = parent->GetChildren();
    auto const it = ranges::find_if(children, [&](auto const& child) { return child == m_selection; });
    auto const oldIndex = static_cast<int>(it - std::begin(children));

    if (oldIndex == 0) {
        return;
    }

    auto const newIndex = oldIndex - 1;
    auto changeAction = std::make_unique<ChangeIndexAction>(m_selection, oldIndex, newIndex);
    changeAction->Do();
    AddEditAction(std::move(changeAction));
}

void EditorLayer::MoveSelectionDown() {
    if (!m_selection || !m_selection->GetParent()) {
        return;
    }

    auto parent = m_selection->GetParent();
    auto& children = parent->GetChildren();
    auto const it = ranges::find_if(children, [&](auto const& child) { return child == m_selection; });
    auto const oldIndex = static_cast<int>(it - std::begin(children));

    if (oldIndex == static_cast<int>(children.size() - 1)) {
        return;
    }

    auto const newIndex = oldIndex + 1;
    auto changeAction = std::make_unique<ChangeIndexAction>(m_selection, oldIndex, newIndex);
    changeAction->Do();
    AddEditAction(std::move(changeAction));
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
        case moth_ui::Key::Pageup:
            MoveSelectionUp();
            return true;
        case moth_ui::Key::Pagedown:
            MoveSelectionDown();
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
        float const scaleFactor = 100.0f / m_canvasProperties.m_zoom;
        m_canvasProperties.m_offset += event.GetDelta() / scaleFactor;
    }
    return false;
}

bool EditorLayer::OnMouseWheel(moth_ui::EventMouseWheel const& event) {
    float const scaleFactor = 100.0f / m_canvasProperties.m_zoom;
    m_canvasProperties.m_zoom += static_cast<int>(event.GetDelta().y * 6 / scaleFactor);
    return false;
}

bool EditorLayer::OnRequestQuitEvent(EventRequestQuit const& event) {
    if (IsWorkPending()) {
        m_confirmPrompt.SetTitle("Exit?");
        m_confirmPrompt.SetMessage("You have unsaved work? Exit?");
        m_confirmPrompt.SetPositiveText("Exit");
        m_confirmPrompt.SetNegativeText("Cancel");
        m_confirmPrompt.SetPositiveAction([this]() {
            m_layerStack->BroadcastEvent(EventQuit());
        });
        m_confirmPrompt.SetNegativeAction([]() {
        });
        m_confirmPrompt.Open();
    } else {
        m_layerStack->BroadcastEvent(EventQuit());
    }
    return true;
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
    auto& tracks = entity->m_tracks;
    int const frameNo = m_selectedFrame;
    std::unique_ptr<CompositeAction> editAction = std::make_unique<CompositeAction>();

    auto const SetTrackValue = [&](moth_ui::AnimationTrack::Target target, float value) {
        auto& track = tracks.at(target);
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
    auto& tracks = entity->m_tracks;
    int const frameNo = m_selectedFrame;
    std::unique_ptr<CompositeAction> editAction = std::make_unique<CompositeAction>();

    auto const SetTrackValue = [&](moth_ui::AnimationTrack::Target target, float value) {
        auto& track = tracks.at(target);
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