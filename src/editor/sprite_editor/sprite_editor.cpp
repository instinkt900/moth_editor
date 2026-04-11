#include "common.h"
#include "sprite_editor.h"
#include "editor/editor_layer.h"

#include <nfd.h>

SpriteEditor::SpriteEditor(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

void SpriteEditor::DrawDataEditor() {
    // Read-only path display
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputText("##sprite_path", m_pathBuffer, sizeof(m_pathBuffer) - 1, ImGuiInputTextFlags_ReadOnly);

    if (!m_spriteSheet) {
        ImGui::TextDisabled("Use File > Load to open a sprite sheet.");
        return;
    }

    ImGui::Separator();

    float const totalH = ImGui::GetContentRegionAvail().y;
    float const framesH = std::floor(totalH * 0.5f);

    if (ImGui::BeginChild("##frames_pane", ImVec2(0, framesH), ImGuiChildFlags_None)) {
        DrawFramesPane();
    }
    ImGui::EndChild();

    ImGui::BeginChild("##clips_pane", ImVec2(0, 0), ImGuiChildFlags_None);
    DrawClipsPane();
    ImGui::EndChild();
}

void SpriteEditor::Draw() {
    if (!m_open) {
        return;
    }

    auto const doLoad = [this]() {
        nfdchar_t* outPath = nullptr;
        std::string const currentPath = std::filesystem::current_path().string();
        if (NFD_OpenDialog("json", currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
            strncpy(m_pathBuffer, outPath, sizeof(m_pathBuffer) - 1);
            m_pathBuffer[sizeof(m_pathBuffer) - 1] = '\0';
            NFD_Free(outPath);
            LoadSpriteSheet(m_pathBuffer);
        }
    };

    ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Sprite Editor", &m_open, ImGuiWindowFlags_MenuBar)) {
        // Keyboard shortcuts (only when this window or its children are focused)
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            auto const& io = ImGui::GetIO();
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
                UndoSpriteAction();
            }
            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                RedoSpriteAction();
            }
        }

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    ClearSpriteActions();
                    m_pathBuffer[0]      = '\0';
                    m_imagePathBuffer[0] = '\0';
                    m_frames.clear();
                    m_clips.clear();
                    m_selectedFrame   = -1;
                    m_selectedClip    = -1;
                    m_clipPlaying     = false;
                    m_clipCurrentStep = 0;
                    m_clipElapsedMs   = 0.0f;
                    m_zoom            = 1.0f;
                    m_spriteSheet     = std::make_shared<moth_graphics::graphics::SpriteSheet>(
                        nullptr,
                        std::vector<moth_graphics::graphics::SpriteSheet::FrameEntry>{},
                        std::vector<moth_graphics::graphics::SpriteSheet::ClipEntry>{});
                }
                if (ImGui::MenuItem("Load...")) {
                    doLoad();
                }
                bool const hasImage = m_imagePathBuffer[0] != '\0';
                bool const hasPath  = m_pathBuffer[0] != '\0';
                if (ImGui::MenuItem("Save", nullptr, false, hasImage && hasPath)) {
                    SaveSpriteSheet();
                }
                if (ImGui::MenuItem("Save As...", nullptr, false, hasImage)) {
                    nfdchar_t* outPath = nullptr;
                    std::string const currentPath = std::filesystem::current_path().string();
                    if (NFD_SaveDialog("json", currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
                        strncpy(m_pathBuffer, outPath, sizeof(m_pathBuffer) - 1);
                        m_pathBuffer[sizeof(m_pathBuffer) - 1] = '\0';
                        NFD_Free(outPath);
                        SaveSpriteSheet();
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Import Sheet...", nullptr, false, m_spriteSheet != nullptr)) {
                    nfdchar_t* outPath = nullptr;
                    std::string const currentPath = std::filesystem::current_path().string();
                    if (NFD_OpenDialog("png,jpg,jpeg,bmp", currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
                        std::filesystem::path const imagePath = outPath;
                        NFD_Free(outPath);
                        ImportSheet(imagePath);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit")) {
                    m_open = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                bool const canUndo = m_undoIndex >= 0;
                bool const canRedo = m_undoIndex < static_cast<int>(m_undoStack.size()) - 1;
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) {
                    UndoSpriteAction();
                }
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) {
                    RedoSpriteAction();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Preferences")) {
                auto& cfg = m_editorLayer.GetConfig();
                ImGui::ColorEdit4("Normal border##pref",   cfg.SpriteEditorNormalColor.data,   ImGuiColorEditFlags_NoInputs);
                ImGui::ColorEdit4("Selected border##pref", cfg.SpriteEditorSelectedColor.data, ImGuiColorEditFlags_NoInputs);
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputInt("Border thickness##pref", &cfg.SpriteEditorRectThickness);
                cfg.SpriteEditorRectThickness = std::max(1, cfg.SpriteEditorRectThickness);
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::BeginTable("##sprite_layout", 2,
                ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Editor",  ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            DrawPreview();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("##sprite_data", ImVec2(0, 0), ImGuiChildFlags_None);
            DrawDataEditor();
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
