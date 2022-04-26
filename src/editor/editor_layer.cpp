#include "common.h"
#include "editor_layer.h"
#include "app.h"
#include "bounds_widget.h"

#include "actions/add_action.h"

#include "panels/editor_panel_asset_list.h"
#include "panels/editor_panel_canvas_properties.h"
#include "panels/editor_panel_project_properties.h"
#include "panels/editor_panel_properties.h"
#include "panels/editor_panel_elements.h"
#include "panels/editor_panel_animation.h"
#include "panels/editor_panel_keyframes.h"
#include "panels/editor_panel_undo_stack.h"
#include "panels/editor_panel_preview.h"
#include "panels/editor_panel_canvas.h"

#include "editor/actions/delete_action.h"
#include "editor/actions/composite_action.h"
#include "editor/actions/modify_keyframe_action.h"
#include "editor/actions/add_keyframe_action.h"
#include "editor/actions/change_index_action.h"
#include "editor/actions/editor_action.h"

#include "moth_ui/layout/layout.h"
#include "moth_ui/group.h"
#include "moth_ui/event_dispatch.h"

#include "imgui-filebrowser/imfilebrowser.h"

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

EditorLayer::EditorLayer() {
    m_layoutProject.m_layoutRoot = ".";
    m_layoutProject.m_imageRoot = ".";

    AddEditorPanel<EditorPanelCanvas>(*this, true);
    AddEditorPanel<EditorPanelCanvasProperties>(*this, true);
    AddEditorPanel<EditorPanelProjectProperties>(*this, false);
    AddEditorPanel<EditorPanelAssetList>(*this, true);
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

bool EditorLayer::OnEvent(moth_ui::Event const& event) {
    moth_ui::EventDispatch dispatch(event);
    dispatch.Dispatch(this, &EditorLayer::OnKey);
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
    m_rootDockId = ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(m_lastErrorMsg.c_str());
        ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);
        if (ImGui::Button("OK", button_size)) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if (m_errorPending) {
        ImGui::OpenPopup("Error");
        m_errorPending = false;
    }

    m_confirmPrompt.Draw();

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
            if (ImGui::MenuItem("New Layout", "Ctrl+N")) {
                MenuFuncNewLayout();
            }
            if (ImGui::MenuItem("Open Layout", "Ctrl+O")) {
                MenuFuncOpenLayout();
            }
            if (ImGui::MenuItem("Save Layout", "Ctrl+S", nullptr, !m_currentLayoutPath.empty())) {
                MenuFuncSaveLayout();
            }
            if (ImGui::MenuItem("Save Layout As", "Ctrl+Shift+S")) {
                MenuFuncSaveLayoutAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Open Project..")) {
                MenuFuncOpenProject();
            }
            if (ImGui::MenuItem("Save Project", nullptr, nullptr, !m_layoutProject.m_loadedPath.empty())) {
                MenuFuncSaveProject();
            }
            if (ImGui::MenuItem("Save Project As..")) {
                MenuFuncSaveProjectAs();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                m_layerStack->BroadcastEvent(EventQuit{});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, m_selection != nullptr)) {
                CutEntity();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, m_selection != nullptr)) {
                CopyEntity();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, m_copiedEntity != nullptr)) {
                PasteEntity();
            }
            if (ImGui::MenuItem("Delete", "Del", nullptr, m_copiedEntity != nullptr)) {
                DeleteEntity();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Canvas", "F")) {
                ResetCanvas();
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("Panels")) {
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
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
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

    GetEditorPanel<EditorPanelAssetList>()->Refresh();

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
        std::shared_ptr<moth_ui::Layout> newLayout;
        auto const loadResult = moth_ui::Layout::Load(path, &newLayout);
        if (loadResult == moth_ui::Layout::LoadResult::Success) {
            m_rootLayout = newLayout;
            m_currentLayoutPath = path;
            m_selectedFrame = 0;
            m_selection = nullptr;
            ClearEditActions();
            Rebuild();
        } else {
            if (loadResult == moth_ui::Layout::LoadResult::DoesNotExist) {
                ShowError("File not found.");
            } else if (loadResult == moth_ui::Layout::LoadResult::IncorrectFormat) {
                ShowError("File was not valid.");
            }
        }
    }
}

void EditorLayer::SaveLayout(char const* path) {
    if (m_rootLayout->Save(path)) {
        m_lastSaveActionIndex = m_actionIndex;
        m_currentLayoutPath = path;
    }
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

void EditorLayer::MenuFuncNewLayout() {
    NewLayout();
}

void EditorLayer::MenuFuncOpenLayout() {
    s_fileDialog.SetTitle("Open Layout..");
    s_fileDialog.SetTypeFilters({ moth_ui::Layout::Extension });
    s_fileDialog.Open();
    s_fileOpenMode = FileOpenMode::OpenLayout;
}

void EditorLayer::MenuFuncSaveLayout() {
    if (!m_currentLayoutPath.empty()) {
        SaveLayout(m_currentLayoutPath.c_str());
    }
}

void EditorLayer::MenuFuncSaveLayoutAs() {
    s_fileDialog.SetTitle("Save Layout As..");
    s_fileDialog.SetTypeFilters({ moth_ui::Layout::Extension });
    s_fileDialog.Open();
    s_fileOpenMode = FileOpenMode::SaveLayout;
}

void EditorLayer::MenuFuncOpenProject() {
    s_fileDialog.SetTitle("Open Project..");
    s_fileDialog.SetTypeFilters({ ".json" });
    s_fileDialog.Open();
    s_fileOpenMode = FileOpenMode::OpenProject;
}

void EditorLayer::MenuFuncSaveProject() {
    if (!m_layoutProject.m_loadedPath.empty()) {
        SaveProject(m_layoutProject.m_loadedPath.c_str());
    }
}

void EditorLayer::MenuFuncSaveProjectAs() {
    s_fileDialog.SetTitle("Save Project..");
    s_fileDialog.SetTypeFilters({ ".json" });
    s_fileDialog.Open();
    s_fileOpenMode = FileOpenMode::SaveProject;
}

void EditorLayer::CopyEntity() {
    if (m_selection) {
        m_copiedEntity = m_selection->GetLayoutEntity();
    }
}

void EditorLayer::CutEntity() {
    CopyEntity();
    DeleteEntity();
}

void EditorLayer::PasteEntity() {
    if (m_copiedEntity) {
        auto copiedEntity = m_copiedEntity->Clone();
        auto copyInstance = copiedEntity->Instantiate();
        auto addAction = std::make_unique<AddAction>(std::move(copyInstance), m_root);
        PerformEditAction(std::move(addAction));
        m_root->RecalculateBounds();
    }
}

void EditorLayer::DeleteEntity() {
    if (m_selection) {
        PerformEditAction(std::make_unique<DeleteAction>(m_selection, m_root));
        SetSelection(nullptr);
    }
}

void EditorLayer::ResetCanvas() {
    m_canvasProperties = {};
}

bool EditorLayer::OnKey(moth_ui::EventKey const& event) {
    if (event.GetAction() == moth_ui::KeyAction::Up) {
        switch (event.GetKey()) {
        case moth_ui::Key::Delete:
            if (m_selection) {
                PerformEditAction(std::make_unique<DeleteAction>(m_selection, m_root));
                SetSelection(nullptr);
            }
            return true;
        case moth_ui::Key::F:
            ResetCanvas();
            break;
        case moth_ui::Key::N:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                MenuFuncNewLayout();
            }
            break;
        case moth_ui::Key::O:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                MenuFuncOpenLayout();
            }
            break;
        case moth_ui::Key::S: {
            auto const anyCtrlPressed = (event.GetMods() & moth_ui::KeyMod_Ctrl) != 0;
            auto const anyShiftPressed = (event.GetMods() & moth_ui::KeyMod_Shift) != 0;
            if (anyCtrlPressed && anyShiftPressed) {
                MenuFuncSaveLayoutAs();
            } else if (anyCtrlPressed) {
                MenuFuncSaveLayout();
            }
        } break;
        case moth_ui::Key::Z:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                UndoEditAction();
            }
            return true;
        case moth_ui::Key::Y:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                RedoEditAction();
            }
            return true;
        case moth_ui::Key::C:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                CopyEntity();
            }
            return true;
        case moth_ui::Key::X:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                CutEntity();
            }
            return true;
        case moth_ui::Key::V:
            if ((event.GetMods() & moth_ui::KeyMod_Ctrl) != 0) {
                PasteEntity();
            }
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

void EditorLayer::ShowError(std::string const& message) {
    m_lastErrorMsg = message;
    m_errorPending = true;
}
