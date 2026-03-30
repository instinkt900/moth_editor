#include "common.h"
#include "editor_layer.h"
#include "editor_application.h"
#include "bounds_widget.h"

#include "actions/add_action.h"

#include "panels/editor_panel_config.h"
#include "panels/editor_panel_asset_list.h"
#include "panels/editor_panel_canvas_properties.h"
#include "panels/editor_panel_properties.h"
#include "panels/editor_panel_elements.h"
#include "panels/editor_panel_animation.h"
#include "panels/editor_panel_fonts.h"
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
#include "moth_ui/nodes/group.h"
#include "moth_ui/events/event_dispatch.h"
#include "moth_ui/context.h"
#include "moth_ui/animation/keyframe.h"
#include "moth_graphics/platform/window.h"

#include "texture_packer.h"

#include <nfd.h>
#include <ctime>

EditorLayer::EditorLayer(moth_ui::Context& context, moth_graphics::graphics::IGraphics& graphics, EditorApplication* app)
    : m_app(app)
    , m_context(context)
    , m_graphics(graphics) {
    LoadConfig();

    AddEditorPanel<EditorPanelConfig>(*this, false);
    auto* const canvasPanel = AddEditorPanel<EditorPanelCanvas>(*this, true);
    AddEditorPanel<EditorPanelCanvasProperties>(*this, true, *canvasPanel);
    AddEditorPanel<EditorPanelAssetList>(*this, true);
    AddEditorPanel<EditorPanelProperties>(*this, true);
    AddEditorPanel<EditorPanelElements>(*this, true);
    AddEditorPanel<EditorPanelAnimation>(*this, true);
    AddEditorPanel<EditorPanelFonts>(*this, true);
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

    if (m_config.AutoSaveEnabled && !m_currentLayoutPath.empty() && IsWorkPending()) {
        m_autoSaveAccumulatedMs += ticks;
        uint32_t const intervalMs = static_cast<uint32_t>(m_config.AutoSaveIntervalMinutes) * 60u * 1000u;
        if (m_autoSaveAccumulatedMs >= intervalMs) {
            m_autoSaveAccumulatedMs = 0;
            AutoSave();
        }
    } else {
        m_autoSaveAccumulatedMs = 0;
    }

    auto const windowTitle = fmt::format("{}{}", m_currentLayoutPath.empty() ? "New Layout" : m_currentLayoutPath.string(), IsWorkPending() ? " *" : "");
    m_app->GetWindow()->SetWindowTitle(windowTitle);
}

void EditorLayer::Draw() {
    m_rootDockId = ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

    auto* root = ImGui::DockBuilderGetNode(m_rootDockId);
    if (!root->IsSplitNode()) {
        auto dockTop = ImGui::DockBuilderSplitNode(m_rootDockId, ImGuiDir_Up, 0.1f, nullptr, &m_rootDockId);
        auto dockBottom = ImGui::DockBuilderSplitNode(m_rootDockId, ImGuiDir_Down, 0.2f, nullptr, &m_rootDockId);
        auto dockLeft = ImGui::DockBuilderSplitNode(m_rootDockId, ImGuiDir_Left, 0.1f, nullptr, &m_rootDockId);
        auto dockRight = ImGui::DockBuilderSplitNode(m_rootDockId, ImGuiDir_Right, 0.1f, nullptr, &m_rootDockId);

        ImGuiID dockTopLeft = 0;
        ImGuiID dockTopRight = 0;
        ImGui::DockBuilderSplitNode(dockTop, ImGuiDir_Left, 0.5f, &dockTopLeft, &dockTopRight);

        ImGui::DockBuilderGetNode(dockTopLeft)->SetLocalFlags(ImGuiDockNodeFlags_HiddenTabBar);
        ImGui::DockBuilderGetNode(dockTopRight)->SetLocalFlags(ImGuiDockNodeFlags_HiddenTabBar);
        ImGui::DockBuilderGetNode(dockLeft)->SetLocalFlags(ImGuiDockNodeFlags_HiddenTabBar);
        ImGui::DockBuilderGetNode(dockRight)->SetLocalFlags(ImGuiDockNodeFlags_HiddenTabBar);
        ImGui::DockBuilderGetNode(dockBottom)->SetLocalFlags(ImGuiDockNodeFlags_HiddenTabBar);
        auto* rootDock = ImGui::DockBuilderGetNode(m_rootDockId);
        rootDock->SetLocalFlags(rootDock->LocalFlags | ImGuiDockNodeFlags_HiddenTabBar);

        auto* panelCanvasProperties = GetEditorPanel<EditorPanelCanvasProperties>();
        auto* panelAssets = GetEditorPanel<EditorPanelAssetList>();
        auto* panelProperties = GetEditorPanel<EditorPanelProperties>();
        auto* panelElements = GetEditorPanel<EditorPanelElements>();
        auto* panelAnimation = GetEditorPanel<EditorPanelAnimation>();
        auto* panelFonts = GetEditorPanel<EditorPanelFonts>();

        ImGui::DockBuilderDockWindow(panelCanvasProperties->GetTitle().c_str(), dockTopLeft);
        ImGui::DockBuilderDockWindow(panelElements->GetTitle().c_str(), dockTopRight);
        ImGui::DockBuilderDockWindow(panelProperties->GetTitle().c_str(), dockLeft);
        ImGui::DockBuilderDockWindow(panelAssets->GetTitle().c_str(), dockRight);
        ImGui::DockBuilderDockWindow(panelFonts->GetTitle().c_str(), dockRight);
        ImGui::DockBuilderDockWindow(panelAnimation->GetTitle().c_str(), dockBottom);

        ImGui::DockBuilderFinish(m_rootDockId);
    }

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

    if (IsEditorPanelFocused<EditorPanelCanvas>() || IsEditorPanelFocused<EditorPanelAnimation>()) {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
            DeleteEntity();
        } else if (ImGui::IsKeyPressed(ImGuiKey_H)) {
            ToggleEntityVisibility();
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
            if (ImGui::BeginMenu("Open Recent", !m_recentFiles.empty())) {
                for (auto const& recentPath : m_recentFiles) {
                    if (ImGui::MenuItem(recentPath.string().c_str())) {
                        LoadLayout(recentPath);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save Layout", "Ctrl+S", nullptr, !m_currentLayoutPath.empty())) {
                MenuFuncSaveLayout();
            }
            if (ImGui::MenuItem("Save Layout As", "Ctrl+Shift+S")) {
                MenuFuncSaveLayoutAs();
            }
            ImGui::Separator();
            ImGui::Checkbox("Autosave", &m_config.AutoSaveEnabled);
            if (m_config.AutoSaveEnabled) {
                ImGui::SetNextItemWidth(80.0f);
                ImGui::InputInt("Interval (min)", &m_config.AutoSaveIntervalMinutes);
                m_config.AutoSaveIntervalMinutes = std::max(1, m_config.AutoSaveIntervalMinutes);
                ImGui::SetNextItemWidth(80.0f);
                ImGui::InputInt("Max Versions", &m_config.AutoSaveMaxVersions);
                m_config.AutoSaveMaxVersions = std::max(1, m_config.AutoSaveMaxVersions);
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                m_layerStack->FireEvent(moth_graphics::EventRequestQuit{});
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
            ImGui::Separator();
            ImGui::Checkbox("Snap to Grid", &m_config.SnapToGrid);
            ImGui::Checkbox("Snap to Angle", &m_config.SnapToAngle);
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputFloat("Snap Angle", &m_config.SnapAngle, 0.0f, 0.0f, "%.1f");
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

void EditorLayer::OnAddedToStack(moth_ui::LayerStack* layerStack) {
    Layer::OnAddedToStack(layerStack);
    NewLayout();
    CheckCrashRecovery();
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
    if (IsWorkPending()) {
        SaveCrashRecovery();
    }
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
    m_lastSaveActionIndex = -1;
}

void EditorLayer::NewLayout(bool discard) {
    if (!discard && IsWorkPending()) {
        m_confirmPrompt.SetTitle("Unsaved Changes");
        m_confirmPrompt.SetMessage("You have unsaved changes. What would you like to do?");
        m_confirmPrompt.SetPositiveText("Save and New");
        m_confirmPrompt.SetNegativeText("Discard and New");
        m_confirmPrompt.SetCancelText("Cancel");
        m_confirmPrompt.SetPositiveAction([this]() {
            SaveLayout(m_currentLayoutPath.c_str());
            NewLayout();
        });
        m_confirmPrompt.SetNegativeAction([this]() {
            NewLayout(true);
        });
        m_confirmPrompt.SetCancelAction(nullptr);
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
            panel->OnNewLayout();
            panel->OnLayoutLoaded();
        }
    }
}

void EditorLayer::LoadLayout(std::filesystem::path const& path, bool discard) {
    if (!discard && IsWorkPending()) {
        m_confirmPrompt.SetTitle("Unsaved Changes");
        m_confirmPrompt.SetMessage("You have unsaved changes. What would you like to do?");
        m_confirmPrompt.SetPositiveText("Save and Load");
        m_confirmPrompt.SetNegativeText("Discard and Load");
        m_confirmPrompt.SetCancelText("Cancel");
        m_confirmPrompt.SetPositiveAction([this, path]() {
            SaveLayout(m_currentLayoutPath);
            LoadLayout(path);
        });
        m_confirmPrompt.SetNegativeAction([this, path]() {
            LoadLayout(path, true);
        });
        m_confirmPrompt.SetCancelAction(nullptr);
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
            AddRecentFile(path);
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
    moth_ui::Layout::SaveOptions saveOptions;
    saveOptions.pretty = true;
    if (m_rootLayout->Save(path, saveOptions)) {
        m_lastSaveActionIndex = m_actionIndex;
        m_currentLayoutPath = path;
        AddRecentFile(path);
    }
}

void EditorLayer::AutoSave() {
    if (m_currentLayoutPath.empty()) {
        return;
    }

    std::time_t const now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", &localTime);

    auto const dir = m_currentLayoutPath.parent_path();
    auto const stem = m_currentLayoutPath.stem().string();
    auto const ext = m_currentLayoutPath.extension().string();
    auto const autosavePath = dir / (stem + ".autosave_" + timeBuf + ext);

    moth_ui::Layout::SaveOptions saveOptions;
    saveOptions.pretty = true;
    m_rootLayout->Save(autosavePath, saveOptions);

    // Collect existing autosave files for this layout
    std::string const prefix = stem + ".autosave_";
    std::vector<std::filesystem::path> autosaveFiles;
    std::error_code ec;
    for (auto const& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) {
            continue;
        }
        auto const name = entry.path().filename().string();
        if (name.substr(0, prefix.size()) == prefix && entry.path().extension() == ext) {
            autosaveFiles.push_back(entry.path());
        }
    }

    // Sort ascending by name — timestamps are fixed-width so lexicographic order = chronological
    std::sort(autosaveFiles.begin(), autosaveFiles.end());

    int const maxVersions = std::max(1, m_config.AutoSaveMaxVersions);
    while (static_cast<int>(autosaveFiles.size()) > maxVersions) {
        std::filesystem::remove(autosaveFiles.front(), ec);
        autosaveFiles.erase(autosaveFiles.begin());
    }
}

std::filesystem::path EditorLayer::GetCrashRecoveryPath() {
    return std::filesystem::temp_directory_path() / "moth_editor_recovery.moth";
}

void EditorLayer::CheckCrashRecovery() {
    std::error_code ec;
    if (!std::filesystem::exists(GetCrashRecoveryPath(), ec)) {
        return;
    }

    m_confirmPrompt.SetTitle("Crash Recovery");
    m_confirmPrompt.SetMessage("A crash recovery file was found. Would you like to restore your previous session?");
    m_confirmPrompt.SetPositiveText("Restore");
    m_confirmPrompt.SetNegativeText("Discard");
    m_confirmPrompt.SetCancelText("");
    m_confirmPrompt.SetPositiveAction([this]() {
        auto const recoveryPath = GetCrashRecoveryPath();
        std::shared_ptr<moth_ui::Layout> recoveredLayout;
        if (moth_ui::Layout::Load(recoveryPath, &recoveredLayout) == moth_ui::Layout::LoadResult::Success) {
            // Restore the original file path stored in extra data
            std::filesystem::path originalPath;
            auto& extraData = recoveredLayout->GetExtraData();
            if (extraData.contains("crash_recovery_original_path")) {
                originalPath = extraData["crash_recovery_original_path"].get<std::string>();
                extraData.erase("crash_recovery_original_path");
            }

            m_rootLayout = recoveredLayout;
            m_currentLayoutPath = originalPath;
            m_selectedFrame = 0;
            ClearSelection();
            ClearEditActions();
            m_lockedNodes.clear();
            Rebuild();
            for (auto&& [panelId, panel] : m_panels) {
                panel->OnLayoutLoaded();
            }
            // Mark as having unsaved work since this is a recovery, not a real save
            m_lastSaveActionIndex = m_actionIndex - 1;
        }
        DeleteCrashRecovery();
    });
    m_confirmPrompt.SetNegativeAction([]() {
        DeleteCrashRecovery();
    });
    m_confirmPrompt.SetCancelAction(nullptr);
    m_confirmPrompt.Open();
}

void EditorLayer::SaveCrashRecovery() {
    if (m_rootLayout == nullptr) {
        return;
    }
    // Temporarily stash the original path in extra data so it can be restored on recovery
    m_rootLayout->GetExtraData()["crash_recovery_original_path"] = m_currentLayoutPath.string();
    moth_ui::Layout::SaveOptions saveOptions;
    saveOptions.pretty = false;
    m_rootLayout->Save(GetCrashRecoveryPath(), saveOptions);
    m_rootLayout->GetExtraData().erase("crash_recovery_original_path");
}

void EditorLayer::DeleteCrashRecovery() { // NOLINT(readability-convert-member-functions-to-static)
    std::error_code ec;
    std::filesystem::remove(GetCrashRecoveryPath(), ec);
}

void EditorLayer::Rebuild() {
    m_root = std::make_unique<moth_ui::Group>(m_context, m_rootLayout);
}

void EditorLayer::MoveSelectionUp() {
    auto compAction = std::make_unique<CompositeAction>();

    // might need to do this more carefully since the order we do this matters
    for (auto&& node : m_selection) {
        if (node == nullptr || node->GetParent() == nullptr) {
            continue;
        }

        auto* parent = node->GetParent();
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
        if (node == nullptr || node->GetParent() == nullptr) {
            continue;
        }

        auto* parent = node->GetParent();
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
    auto const currentPath = std::filesystem::current_path().string();
    nfdchar_t* outPath = NULL;
    nfdresult_t result = NFD_OpenDialog(moth_ui::Layout::Extension.c_str(), currentPath.c_str(), &outPath);

    if (result == NFD_OKAY) {
        LoadLayout(outPath);
    }
}

void EditorLayer::MenuFuncSaveLayout() {
    if (!m_currentLayoutPath.empty()) {
        SaveLayout(m_currentLayoutPath.c_str());
    } else {
        MenuFuncSaveLayoutAs();
    }
}

void EditorLayer::MenuFuncSaveLayoutAs() {
    auto const currentPath = std::filesystem::current_path().string();
    nfdchar_t* outPath = NULL;
    nfdresult_t result = NFD_SaveDialog(moth_ui::Layout::Extension.c_str(), currentPath.c_str(), &outPath);

    if (result == NFD_OKAY) {
        std::filesystem::path filePath = outPath;
        if (!filePath.has_extension()) {
            filePath.replace_extension(moth_ui::Layout::Extension);
        }
        SaveLayout(filePath);
    }
}

void EditorLayer::CopyEntity() {
    m_copiedEntities.clear();
    for (auto&& node : m_selection) {
        m_copiedEntities.push_back(node->GetLayoutEntity()->Clone(moth_ui::LayoutEntity::CloneType::Deep));
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
        auto copyInstance = clonedEntity->Instantiate(m_context);
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

void EditorLayer::ToggleEntityVisibility() {
    auto const selection = GetSelection();
    if (!selection.empty()) {
        bool const visible = !(*selection.begin())->IsVisible();
        std::unique_ptr<CompositeAction> actions = std::make_unique<CompositeAction>();
        for (auto&& node : selection) {
            auto entity = node->GetLayoutEntity();
            auto action = MakeChangeValueAction(entity->m_visible, entity->m_visible, visible, [node]() { node->ReloadEntity(); });
            actions->GetActions().push_back(std::move(action));
        }
        PerformEditAction(std::move(actions));
    }
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

bool EditorLayer::OnRequestQuitEvent(moth_graphics::EventRequestQuit const& event) {
    if (IsWorkPending()) {
        m_confirmPrompt.SetTitle("Unsaved Changes");
        m_confirmPrompt.SetMessage("You have unsaved changes. What would you like to do?");
        m_confirmPrompt.SetPositiveText("Save and Exit");
        m_confirmPrompt.SetNegativeText("Discard and Exit");
        m_confirmPrompt.SetCancelText("Cancel");
        m_confirmPrompt.SetPositiveAction([this]() {
            if (m_currentLayoutPath.empty()) {
                MenuFuncSaveLayoutAs();
                if (!m_currentLayoutPath.empty()) {
                    Shutdown();
                }
            } else {
                SaveLayout(m_currentLayoutPath);
                Shutdown();
            }
        });
        m_confirmPrompt.SetNegativeAction([this]() {
            Shutdown();
        });
        m_confirmPrompt.SetCancelAction(nullptr);
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
    if (node != nullptr) {
        if (m_editBoundsContext.empty()) {
            auto context = std::make_unique<EditBoundsContext>();
            context->node = node;
            context->entity = node->GetLayoutEntity();
            context->originalRect = node->GetLayoutRect();
            context->originalPivot = context->entity ? context->entity->m_pivot : moth_ui::FloatVec2{ 0.5f, 0.5f };
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
                context->originalPivot = context->entity ? context->entity->m_pivot : moth_ui::FloatVec2{ 0.5f, 0.5f };
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
            if (auto* keyframePtr = track->GetKeyframe(frameNo)) {
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

    for (auto&& context : m_editBoundsContext) {
        if (!context->entity) {
            continue;
        }
        auto const finalPivot = context->entity->m_pivot;
        if (finalPivot != context->originalPivot) {
            auto node = context->node;
            auto entity = context->entity;
            auto const originalPivot = context->originalPivot;
            editAction->GetActions().push_back(std::make_unique<BasicAction>(
                [entity, node, finalPivot]() {
                    entity->m_pivot = finalPivot;
                    node->SetPivot(finalPivot);
                    node->RecalculateBounds();
                },
                [entity, node, originalPivot]() {
                    entity->m_pivot = originalPivot;
                    node->SetPivot(originalPivot);
                    node->RecalculateBounds();
                }
            ));
        }
    }

    if (!editAction->GetActions().empty()) {
        AddEditAction(std::move(editAction));
    }

    m_editBoundsContext.clear();
}

void EditorLayer::BeginEditColor(std::shared_ptr<moth_ui::Node> node) {
    if (m_editColorContext == nullptr && node != nullptr) {
        m_editColorContext = std::make_unique<EditColorContext>();
        m_editColorContext->node = node;
        m_editColorContext->entity = node->GetLayoutEntity();
        m_editColorContext->originalColor = node->GetColor();
    } else {
        assert(m_editColorContext->entity == node->GetLayoutEntity());
    }
}

void EditorLayer::EndEditColor() {
    if (m_editColorContext == nullptr) {
        return;
    }
    auto entity = m_editColorContext->entity;
    auto& tracks = entity->m_tracks;
    int const frameNo = m_selectedFrame;
    std::unique_ptr<CompositeAction> editAction = std::make_unique<CompositeAction>();

    auto const SetTrackValue = [&](moth_ui::AnimationTrack::Target target, float value) {
        auto& track = tracks.at(target);
        if (auto* keyframePtr = track->GetKeyframe(frameNo)) {
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

void EditorLayer::BeginEditRotation(std::shared_ptr<moth_ui::Node> node) {
    if (m_editRotationContext == nullptr && node != nullptr) {
        m_editRotationContext = std::make_unique<EditRotationContext>();
        m_editRotationContext->node = node;
        m_editRotationContext->entity = node->GetLayoutEntity();
        m_editRotationContext->originalRotation = node->GetRotation();
    } else {
        assert(m_editRotationContext->entity == node->GetLayoutEntity());
    }
}

void EditorLayer::EndEditRotation() {
    if (m_editRotationContext == nullptr) {
        return;
    }
    auto entity = m_editRotationContext->entity;
    auto& tracks = entity->m_tracks;
    int const frameNo = m_selectedFrame;
    float const newRotation = m_editRotationContext->node->GetRotation();
    float const delta = newRotation - m_editRotationContext->originalRotation;

    if (delta != 0.0f) {
        auto& track = tracks.at(moth_ui::AnimationTrack::Target::Rotation);
        std::unique_ptr<IEditorAction> editAction;
        if (auto* keyframePtr = track->GetKeyframe(frameNo)) {
            auto const oldValue = keyframePtr->m_value;
            keyframePtr->m_value = newRotation;
            editAction = std::make_unique<ModifyKeyframeAction>(entity, moth_ui::AnimationTrack::Target::Rotation, frameNo, oldValue, newRotation, keyframePtr->m_interpType, keyframePtr->m_interpType);
        } else {
            auto& keyframe = track->GetOrCreateKeyframe(frameNo);
            keyframe.m_value = newRotation;
            editAction = std::make_unique<AddKeyframeAction>(entity, moth_ui::AnimationTrack::Target::Rotation, frameNo, newRotation, moth_ui::InterpType::Linear);
        }
        AddEditAction(std::move(editAction));
    }

    m_editRotationContext.reset();
}

void EditorLayer::ShowError(std::string const& message) {
    m_lastErrorMsg = message;
    m_errorPending = true;
}

void EditorLayer::Shutdown() {
    for (auto&& [panelId, panel] : m_panels) {
        panel->OnShutdown();
    }
    DeleteCrashRecovery();
    SaveConfig();
    m_layerStack->FireEvent(moth_graphics::EventQuit());
}

void EditorLayer::AddRecentFile(std::filesystem::path const& path) {
    auto const absPath = std::filesystem::absolute(path);
    m_recentFiles.erase(std::remove(m_recentFiles.begin(), m_recentFiles.end(), absPath), m_recentFiles.end());
    m_recentFiles.insert(m_recentFiles.begin(), absPath);
    if (static_cast<int>(m_recentFiles.size()) > MaxRecentFiles) {
        m_recentFiles.resize(MaxRecentFiles);
    }
}

void EditorLayer::SaveConfig() {
    auto& j = m_app->GetPersistentState();
    j["editor_config"] = m_config;
    j["font_project"] = m_context.GetFontFactory().GetCurrentProjectPath();
    auto recentArray = nlohmann::json::array();
    for (auto const& p : m_recentFiles) {
        recentArray.push_back(p.string());
    }
    j["recent_files"] = recentArray;
}

void EditorLayer::LoadConfig() {
    auto& j = m_app->GetPersistentState();
    if (!j.is_null()) {
        m_config = j.value("editor_config", m_config);

        std::filesystem::path fontProjectPath = j.value("font_project", "");
        if (!fontProjectPath.empty()) {
            m_context.GetFontFactory().LoadProject(fontProjectPath);
        }

        if (j.contains("recent_files")) {
            auto const& recentFiles = j["recent_files"];
            for (auto it = recentFiles.rbegin(); it != recentFiles.rend(); ++it) {
                AddRecentFile(std::filesystem::path(it->get<std::string>()));
            }
        }
    }
}
