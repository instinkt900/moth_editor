#include "common.h"
#include "editor_layer.h"
#include "backend/iapplication.h"
#include "bounds_widget.h"

#include "actions/add_action.h"

#include "panels/editor_panel_config.h"
#include "panels/editor_panel_asset_list.h"
#include "panels/editor_panel_canvas_properties.h"
#include "panels/editor_panel_properties.h"
#include "panels/editor_panel_elements.h"
#include "panels/editor_panel_animation.h"
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

#include "texture_packer.h"

#include "imgui-filebrowser/imfilebrowser.h"

extern backend::IApplication* g_App;

namespace {
    enum class FileOpenMode {
        Unknown,
        OpenLayout,
        SaveLayout,
    };
    static FileOpenMode s_fileOpenMode = FileOpenMode::Unknown;
    static ImGui::FileBrowser s_fileDialog(ImGuiFileBrowserFlags_EnterNewFilename);
}

EditorLayer::EditorLayer() {
    LoadConfig();

    AddEditorPanel<EditorPanelConfig>(*this, false);
    auto const canvasPanel = AddEditorPanel<EditorPanelCanvas>(*this, true);
    AddEditorPanel<EditorPanelCanvasProperties>(*this, true, *canvasPanel);
    AddEditorPanel<EditorPanelAssetList>(*this, true);
    AddEditorPanel<EditorPanelProperties>(*this, true);
    AddEditorPanel<EditorPanelElements>(*this, true);
    AddEditorPanel<EditorPanelUndoStack>(*this, false);
    AddEditorPanel<EditorPanelPreview>(*this, false);

    for (auto& [type, panel] : m_panels) {
        panel->Refresh();
    }

    m_texturePacker = std::make_unique<TexturePacker>();
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

    auto const windowTitle = fmt::format("{}{}", m_currentLayoutPath.empty() ? "New Layout" : m_currentLayoutPath.string(), IsWorkPending() ? " *" : "");
    g_App->SetWindowTitle(windowTitle);
}

void EditorLayer::Draw() {
    m_rootDockId = ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", m_lastErrorMsg.c_str());
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
        if (s_fileOpenMode == FileOpenMode::OpenLayout) {
            auto const path = s_fileDialog.GetSelected();
            //std::filesystem::current_path(path.parent_path());
            LoadLayout(path);
            s_fileDialog.ClearSelected();
        } else if (s_fileOpenMode == FileOpenMode::SaveLayout) {
            auto filePath = s_fileDialog.GetSelected();
            if (!filePath.has_extension()) {
                filePath.replace_extension(moth_ui::Layout::Extension);
            }
            SaveLayout(filePath);
            s_fileDialog.ClearSelected();
        }
    }

    m_texturePacker->Draw();
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
            if (ImGui::MenuItem("Exit")) {
                m_layerStack->BroadcastEvent(EventRequestQuit{});
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Cut", "Ctrl+X", nullptr, !m_selection.empty())) {
                CutEntity();
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, !m_selection.empty())) {
                CopyEntity();
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, !m_copiedEntities.empty())) {
                PasteEntity();
            }
            if (ImGui::MenuItem("Delete", "Del", nullptr, !m_copiedEntities.empty())) {
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
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Texture Packer")) {
                m_texturePacker->Open();
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
        child->Refresh(static_cast<float>(m_selectedFrame));
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
        m_currentLayoutPath.clear();
        m_selectedFrame = 0;
        ClearSelection();
        ClearEditActions();
        m_lockedNodes.clear();
        Rebuild();
        for (auto&& [panelId, panel] : m_panels) {
            panel->OnLayoutLoaded();
        }
    }
}

void EditorLayer::LoadLayout(std::filesystem::path const& path, bool discard) {
    if (!discard && IsWorkPending()) {
        m_confirmPrompt.SetTitle("Save?");
        m_confirmPrompt.SetMessage("You have unsaved work? Save?");
        m_confirmPrompt.SetPositiveText("Save");
        m_confirmPrompt.SetNegativeText("Discard");
        m_confirmPrompt.SetPositiveAction([this, path]() {
            SaveLayout(m_currentLayoutPath);
            LoadLayout(path);
        });
        m_confirmPrompt.SetNegativeAction([this, path]() {
            LoadLayout(path, true);
        });
        m_confirmPrompt.Open();
    } else {
        std::shared_ptr<moth_ui::Layout> newLayout;
        auto const loadResult = moth_ui::Layout::Load(path, &newLayout);
        if (loadResult == moth_ui::Layout::LoadResult::Success) {
            m_rootLayout = newLayout;
            m_currentLayoutPath = path;
            m_selectedFrame = 0;
            ClearSelection();
            ClearEditActions();
            m_lockedNodes.clear();
            Rebuild();
            for (auto&& [panelId, panel] : m_panels) {
                panel->OnLayoutLoaded();
            }
        } else {
            if (loadResult == moth_ui::Layout::LoadResult::DoesNotExist) {
                ShowError("File not found.");
            } else if (loadResult == moth_ui::Layout::LoadResult::IncorrectFormat) {
                ShowError("File was not valid.");
            }
        }
    }
}

void EditorLayer::SaveLayout(std::filesystem::path const& path) {
    if (m_rootLayout->Save(path)) {
        m_lastSaveActionIndex = m_actionIndex;
        m_currentLayoutPath = path;
    }
}

void EditorLayer::Rebuild() {
    m_root = std::make_unique<moth_ui::Group>(m_rootLayout);
}

void EditorLayer::MoveSelectionUp() {
    auto compAction = std::make_unique<CompositeAction>();

    // might need to do this more carefully since the order we do this matters
    for (auto&& node : m_selection) {
        if (!node || !node->GetParent()) {
            continue;
        }

        auto parent = node->GetParent();
        auto& children = parent->GetChildren();
        auto const it = ranges::find_if(children, [&](auto const& child) { return child == node; });
        auto const oldIndex = static_cast<int>(it - std::begin(children));

        if (oldIndex == 0) {
            continue;
        }

        auto const newIndex = oldIndex - 1;
        auto changeAction = std::make_unique<ChangeIndexAction>(node, oldIndex, newIndex);
        changeAction->Do();
        compAction->GetActions().push_back(std::move(changeAction));
    }

    if (!compAction->GetActions().empty()) {
        AddEditAction(std::move(compAction));
    }
}

void EditorLayer::MoveSelectionDown() {
    auto compAction = std::make_unique<CompositeAction>();

    // might need to do this more carefully since the order we do this matters
    for (auto&& node : m_selection) {
        if (!node || !node->GetParent()) {
            continue;
        }

        auto parent = node->GetParent();
        auto& children = parent->GetChildren();
        auto const it = ranges::find_if(children, [&](auto const& child) { return child == node; });
        auto const oldIndex = static_cast<int>(it - std::begin(children));

        if (oldIndex == static_cast<int>(children.size() - 1)) {
            continue;
        }

        auto const newIndex = oldIndex + 1;
        auto changeAction = std::make_unique<ChangeIndexAction>(node, oldIndex, newIndex);
        changeAction->Do();
        compAction->GetActions().push_back(std::move(changeAction));
    }

    if (!compAction->GetActions().empty()) {
        AddEditAction(std::move(compAction));
    }
}

void EditorLayer::MenuFuncNewLayout() {
    NewLayout();
}

void EditorLayer::MenuFuncOpenLayout() {
    s_fileDialog.SetTitle("Open Layout..");
    s_fileDialog.SetTypeFilters({ moth_ui::Layout::Extension });
    s_fileDialog.SetPwd();
    s_fileDialog.Open();
    s_fileOpenMode = FileOpenMode::OpenLayout;
}

void EditorLayer::MenuFuncSaveLayout() {
    if (!m_currentLayoutPath.empty()) {
        SaveLayout(m_currentLayoutPath.c_str());
    } else {
        MenuFuncSaveLayoutAs();
    }
}

void EditorLayer::MenuFuncSaveLayoutAs() {
    s_fileDialog.SetTitle("Save Layout As..");
    s_fileDialog.SetTypeFilters({ moth_ui::Layout::Extension });
    s_fileDialog.SetPwd();
    s_fileDialog.Open();
    s_fileOpenMode = FileOpenMode::SaveLayout;
}

void EditorLayer::CopyEntity() {
    m_copiedEntities.clear();
    for (auto&& node : m_selection) {
        m_copiedEntities.push_back(node->GetLayoutEntity());
    }
}

void EditorLayer::CutEntity() {
    CopyEntity();
    DeleteEntity();
}

void EditorLayer::PasteEntity() {
    auto compAction = std::make_unique<CompositeAction>();
    for (auto&& copiedEntity : m_copiedEntities) {
        auto clonedEntity = copiedEntity->Clone(moth_ui::LayoutEntity::CloneType::Deep);
        auto copyInstance = clonedEntity->Instantiate();
        auto addAction = std::make_unique<AddAction>(std::move(copyInstance), m_root);
        compAction->GetActions().push_back(std::move(addAction));
    }

    if (!compAction->GetActions().empty()) {
        PerformEditAction(std::move(compAction));
        m_root->RecalculateBounds();
    }
}

void EditorLayer::DeleteEntity() {
    auto compAction = std::make_unique<CompositeAction>();
    for (auto&& node : m_selection) {
        compAction->GetActions().push_back(std::make_unique<DeleteAction>(node, m_root));
    }

    if (!compAction->GetActions().empty()) {
        PerformEditAction(std::move(compAction));
        m_root->RecalculateBounds();
    }

    ClearSelection();
}

void EditorLayer::ResetCanvas() {
    GetEditorPanel<EditorPanelCanvas>()->ResetView();
}

bool EditorLayer::OnKey(moth_ui::EventKey const& event) {
    if (event.GetAction() == moth_ui::KeyAction::Up) {
        switch (event.GetKey()) {
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
	default:
	    return false;
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
            Shutdown();
        });
        m_confirmPrompt.SetNegativeAction([]() {
        });
        m_confirmPrompt.Open();
    } else {
        Shutdown();
    }
    return true;
}

void EditorLayer::ClearSelection() {
    m_selection.clear();
}

void EditorLayer::AddSelection(std::shared_ptr<moth_ui::Node> node) {
    if (!m_editBoundsContext.empty()) {
        EndEditBounds();
    }
    m_selection.insert(node);
}

void EditorLayer::RemoveSelection(std::shared_ptr<moth_ui::Node> node) {
    if (!m_editBoundsContext.empty()) {
        EndEditBounds();
    }
    m_selection.erase(node);
}

bool EditorLayer::IsSelected(std::shared_ptr<moth_ui::Node> node) const {
    return std::end(m_selection) != m_selection.find(node);
}

void EditorLayer::LockNode(std::shared_ptr<moth_ui::Node> node) {
    m_lockedNodes.insert(node);
}

void EditorLayer::UnlockNode(std::shared_ptr<moth_ui::Node> node) {
    m_lockedNodes.erase(node);
}

bool EditorLayer::IsLocked(std::shared_ptr<moth_ui::Node> node) const {
    return std::end(m_lockedNodes) != m_lockedNodes.find(node);
}

void EditorLayer::BeginEditBounds(std::shared_ptr<moth_ui::Node> node) {
    if (node) {
        if (m_editBoundsContext.empty()) {
            auto context = std::make_unique<EditBoundsContext>();
            context->node = node;
            context->entity = node->GetLayoutEntity();
            context->originalRect = node->GetLayoutRect();
            m_editBoundsContext.push_back(std::move(context));
        } else {
            assert((*m_editBoundsContext.begin())->entity == node->GetLayoutEntity());
        }
    } else {
        if (m_editBoundsContext.empty()) {
            for (auto&& selectedNode : m_selection) {
                auto context = std::make_unique<EditBoundsContext>();
                context->node = selectedNode;
                context->entity = selectedNode->GetLayoutEntity();
                context->originalRect = selectedNode->GetLayoutRect();
                m_editBoundsContext.push_back(std::move(context));
            }
        } else {
            assert(m_editBoundsContext.size() == m_selection.size());
        }
    }
}

void EditorLayer::EndEditBounds() {
    if (m_editBoundsContext.empty()) {
        return;
    }

    std::unique_ptr<CompositeAction> editAction = std::make_unique<CompositeAction>();

    for (auto&& context : m_editBoundsContext) {
        auto entity = context->entity;
        auto& tracks = entity->m_tracks;
        int const frameNo = m_selectedFrame;

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
                editAction->GetActions().push_back(std::make_unique<AddKeyframeAction>(entity, target, frameNo, value, moth_ui::InterpType::Linear));
            }
        };

        auto const& newRect = context->node->GetLayoutRect();
        auto const rectDelta = newRect - context->originalRect;

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
    }

    if (!editAction->GetActions().empty()) {
        AddEditAction(std::move(editAction));
    }

    m_editBoundsContext.clear();
}

void EditorLayer::BeginEditColor(std::shared_ptr<moth_ui::Node> node) {
    if (!m_editColorContext && node) {
        m_editColorContext = std::make_unique<EditColorContext>();
        m_editColorContext->node = node;
        m_editColorContext->entity = node->GetLayoutEntity();
        m_editColorContext->originalColor = node->GetColor();
    } else {
        assert(m_editColorContext->entity == node->GetLayoutEntity());
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
            editAction->GetActions().push_back(std::make_unique<AddKeyframeAction>(entity, target, frameNo, value, moth_ui::InterpType::Linear));
        }
    };

    auto const& newColor = m_editColorContext->node->GetColor();
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

void EditorLayer::Shutdown() {
    for (auto&& [panelId, panel] : m_panels) {
        panel->OnShutdown();
    }
    SaveConfig();
    m_layerStack->BroadcastEvent(EventQuit());
}

void EditorLayer::SaveConfig() {
    auto& j = g_App->GetPersistentState();
    j["editor_config"] = m_config;
}

void EditorLayer::LoadConfig() {
    auto& j = g_App->GetPersistentState();
    if (!j.is_null()) {
        m_config = j.value("editor_config", m_config);
    }
}
