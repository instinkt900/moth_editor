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
    m_selectedFrame = -1;
    m_frames.clear();

    auto& assetContext = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();
    m_spriteSheet = assetContext.GetSpriteSheetFactory().GetSpriteSheet(path);
    if (!m_spriteSheet) {
        spdlog::error("Failed to load sprite sheet: {}", path.string());
        return;
    }

    m_frames.reserve(static_cast<size_t>(m_spriteSheet->GetFrameCount()));
    for (int i = 0; i < m_spriteSheet->GetFrameCount(); ++i) {
        moth_graphics::graphics::SpriteSheet::FrameEntry entry;
        m_spriteSheet->GetFrameDesc(i, entry);
        m_frames.push_back(entry);
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

    float const scaleX = (imgW > 0.0f) ? (displayW / imgW) : 1.0f;
    float const scaleY = (imgH > 0.0f) ? (displayH / imgH) : 1.0f;

    ImVec2 const imagePos = ImGui::GetCursorScreenPos();
    image->ImGui({ static_cast<int>(displayW), static_cast<int>(displayH) });

    // Click on the image to select the frame under the cursor
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImVec2 const mouse = ImGui::GetMousePos();
        float const relX = (mouse.x - imagePos.x) / scaleX;
        float const relY = (mouse.y - imagePos.y) / scaleY;

        int hit = -1;
        for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
            auto const& fr = m_frames[i];
            if (relX >= static_cast<float>(fr.rect.x())     &&
                relX <  static_cast<float>(fr.rect.right())  &&
                relY >= static_cast<float>(fr.rect.y())      &&
                relY <  static_cast<float>(fr.rect.bottom())) {
                hit = i;
                break;
            }
        }
        m_selectedFrame = hit;
    }

    // Frame rect overlays
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
        auto const& fr = m_frames[i];
        float const x0 = imagePos.x + (static_cast<float>(fr.rect.x())      * scaleX);
        float const y0 = imagePos.y + (static_cast<float>(fr.rect.y())      * scaleY);
        float const x1 = imagePos.x + (static_cast<float>(fr.rect.right())  * scaleX);
        float const y1 = imagePos.y + (static_cast<float>(fr.rect.bottom()) * scaleY);

        bool const selected = (i == m_selectedFrame);
        ImU32 const color     = selected ? IM_COL32(255, 0, 0, 255) : IM_COL32(255, 255, 0, 255);
        float const thickness = selected ? 2.0f : 2.0f;
        drawList->AddRect({ x0, y0 }, { x1, y1 }, color, 0.0f, 0, thickness);
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

    // ---- Frame list ----
    ImGui::Text("Frames: %d", static_cast<int>(m_frames.size()));

    if (ImGui::CollapsingHeader("Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::BeginTable("##frames_table", 5,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame,
                ImVec2(0.0f, 150.0f))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 30.0f);
            ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("W", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("H", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
                auto const& fr = m_frames[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                ImGui::PushID(i);
                bool const isSelected = (m_selectedFrame == i);
                if (ImGui::Selectable(fmt::format("{}", i).c_str(), isSelected,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                        ImVec2(0, 0))) {
                    m_selectedFrame = isSelected ? -1 : i;
                }
                ImGui::PopID();

                ImGui::TableSetColumnIndex(1); ImGui::Text("%d", fr.rect.x());
                ImGui::TableSetColumnIndex(2); ImGui::Text("%d", fr.rect.y());
                ImGui::TableSetColumnIndex(3); ImGui::Text("%d", fr.rect.w());
                ImGui::TableSetColumnIndex(4); ImGui::Text("%d", fr.rect.h());
            }
            ImGui::EndTable();
        }

        // ---- Frame editor ----
        if (m_selectedFrame >= 0 && m_selectedFrame < static_cast<int>(m_frames.size())) {
            auto& fr = m_frames[m_selectedFrame];
            int x = fr.rect.x();
            int y = fr.rect.y();
            int w = fr.rect.w();
            int h = fr.rect.h();

            ImGui::SeparatorText(fmt::format("Frame {}", m_selectedFrame).c_str());

            float const halfW = (ImGui::GetContentRegionAvail().x
                                 - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            ImGui::SetNextItemWidth(halfW);
            bool const xChanged = ImGui::InputInt("X##fedit", &x);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(halfW);
            bool const yChanged = ImGui::InputInt("Y##fedit", &y);
            ImGui::SetNextItemWidth(halfW);
            bool const wChanged = ImGui::InputInt("W##fedit", &w);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(halfW);
            bool const hChanged = ImGui::InputInt("H##fedit", &h);

            if (xChanged || yChanged || wChanged || hChanged) {
                w = std::max(w, 1);
                h = std::max(h, 1);
                fr.rect = moth_graphics::MakeRect(x, y, w, h);
            }

            int pivotX = fr.pivot.x;
            int pivotY = fr.pivot.y;
            ImGui::SetNextItemWidth(halfW);
            bool const pxChanged = ImGui::InputInt("Pivot X##fedit", &pivotX);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(halfW);
            bool const pyChanged = ImGui::InputInt("Pivot Y##fedit", &pivotY);
            if (pxChanged) { fr.pivot.x = pivotX; }
            if (pyChanged) { fr.pivot.y = pivotY; }
        }
    }

    ImGui::Separator();

    // ---- Clip list (read-only) ----
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
