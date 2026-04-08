#include "common.h"
#include "sprite_editor.h"
#include "editor_layer.h"

#include "moth_graphics/graphics/igraphics.h"
#include "moth_graphics/graphics/surface_context.h"
#include "moth_graphics/graphics/asset_context.h"
#include "moth_graphics/graphics/spritesheet_factory.h"

#include <nfd.h>
#include <spdlog/spdlog.h>

SpriteEditor::SpriteEditor(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

void SpriteEditor::LoadSpriteSheet(std::filesystem::path const& path) {
    auto& assetContext = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();
    m_spriteSheet = assetContext.GetSpriteSheetFactory().GetSpriteSheet(path);
    if (!m_spriteSheet) {
        spdlog::error("Failed to load sprite sheet: {}", path.string());
    }
}

void SpriteEditor::DrawPreview() {
    if (!m_spriteSheet) {
        ImGui::TextDisabled("No sprite sheet loaded.");
        return;
    }

    auto const* image = m_spriteSheet->GetImage().get();
    if (image == nullptr) {
        return;
    }

    ImVec2 const avail = ImGui::GetContentRegionAvail();
    float const imgW = static_cast<float>(image->GetWidth());
    float const imgH = static_cast<float>(image->GetHeight());

    float displayW = avail.x;
    float displayH = (imgW > 0.0f) ? (displayW * imgH / imgW) : avail.y;
    if (avail.y > 0.0f && displayH > avail.y) {
        displayH = avail.y;
        displayW = (imgH > 0.0f) ? (displayH * imgW / imgH) : avail.x;
    }

    ImVec2 const imagePos = ImGui::GetCursorScreenPos();
    image->ImGui({ static_cast<int>(displayW), static_cast<int>(displayH) });

    ImDrawList* const drawList = ImGui::GetWindowDrawList();

    // Overlay yellow rects for each frame
    float const scaleX = (imgW > 0.0f) ? (displayW / imgW) : 1.0f;
    float const scaleY = (imgH > 0.0f) ? (displayH / imgH) : 1.0f;

    for (int i = 0; i < m_spriteSheet->GetFrameCount(); ++i) {
        moth_graphics::graphics::SpriteSheet::FrameEntry frame;
        if (m_spriteSheet->GetFrameDesc(i, frame)) {
            float const x0 = imagePos.x + (static_cast<float>(frame.rect.x()) * scaleX);
            float const y0 = imagePos.y + (static_cast<float>(frame.rect.y()) * scaleY);
            float const x1 = imagePos.x + (static_cast<float>(frame.rect.right()) * scaleX);
            float const y1 = imagePos.y + (static_cast<float>(frame.rect.bottom()) * scaleY);
            drawList->AddRect({ x0, y0 }, { x1, y1 }, IM_COL32(255, 255, 0, 255), 0.0f, 0, 2.0f);
        }
    }
}

void SpriteEditor::DrawDataEditor() {
    // File path input + browse
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
    ImGui::InputText("##sprite_path", m_pathBuffer, sizeof(m_pathBuffer) - 1);
    ImGui::SameLine();
    if (ImGui::Button("...")) {
        nfdchar_t* outPath = nullptr;
        std::string const currentPath = std::filesystem::current_path().string();
        if (NFD_OpenDialog("json", currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
            strncpy(m_pathBuffer, outPath, sizeof(m_pathBuffer) - 1);
            m_pathBuffer[sizeof(m_pathBuffer) - 1] = '\0';
            free(outPath); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory) — NFD allocates with malloc
            LoadSpriteSheet(m_pathBuffer);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (m_pathBuffer[0] != '\0') {
            LoadSpriteSheet(m_pathBuffer);
        }
    }

    if (!m_spriteSheet) {
        return;
    }

    ImGui::Separator();

    int const frameCount = m_spriteSheet->GetFrameCount();
    ImGui::Text("Frames: %d", frameCount);

    if (ImGui::CollapsingHeader("Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##frames_table", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                ImVec2(0.0f, 180.0f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("H", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (int i = 0; i < frameCount; ++i) {
                moth_graphics::graphics::SpriteSheet::FrameEntry frame;
                if (m_spriteSheet->GetFrameDesc(i, frame)) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", i);
                    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", frame.rect.x());
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", frame.rect.y());
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%d", frame.rect.w());
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%d", frame.rect.h());
                }
            }
            ImGui::EndTable();
        }
    }

    ImGui::Separator();

    int const clipCount = m_spriteSheet->GetClipCount();
    ImGui::Text("Clips: %d", clipCount);

    if (ImGui::CollapsingHeader("Clips", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int c = 0; c < clipCount; ++c) {
            auto const clipName = m_spriteSheet->GetClipName(c);
            moth_graphics::graphics::SpriteSheet::ClipDesc clipDesc;
            if (!m_spriteSheet->GetClipDesc(clipName, clipDesc)) {
                continue;
            }

            std::string const nodeLabel = fmt::format("{} ({} steps)###clip_{}", clipName, clipDesc.frames.size(), c);
            if (ImGui::TreeNode(nodeLabel.c_str())) {
                char const* loopStr = nullptr;
                switch (clipDesc.loop) {
                case moth_graphics::graphics::SpriteSheet::LoopType::Stop:  loopStr = "stop";  break;
                case moth_graphics::graphics::SpriteSheet::LoopType::Reset: loopStr = "reset"; break;
                case moth_graphics::graphics::SpriteSheet::LoopType::Loop:  loopStr = "loop";  break;
                }
                ImGui::Text("Loop: %s", loopStr);

                if (!clipDesc.frames.empty()) {
                    ImGui::Text("Frame duration: %d ms", clipDesc.frames[0].durationMs);
                }

                if (ImGui::BeginTable("##clip_steps", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Step",  ImGuiTableColumnFlags_WidthFixed, 40.0f);
                    ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableHeadersRow();

                    for (int f = 0; f < static_cast<int>(clipDesc.frames.size()); ++f) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("%d", f);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", clipDesc.frames[f].frameIndex);
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }
        }
    }
}

void SpriteEditor::Draw() {
    if (!m_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Sprite Editor", &m_open)) {
        if (ImGui::BeginTable("##sprite_layout", 2,
                ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Editor",  ImGuiTableColumnFlags_WidthStretch, 0.4f);
            ImGui::TableNextRow();

            // Preview column — drawn directly in the parent window's draw list
            // so GetWindowDrawList() is the same list the image writes to, with no
            // extra child-window clip rect that could hide the frame overlays.
            ImGui::TableSetColumnIndex(0);
            DrawPreview();

            // Data editor column — child window for independent scrolling
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("##sprite_data", ImVec2(0, 0), ImGuiChildFlags_None);
            DrawDataEditor();
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}
