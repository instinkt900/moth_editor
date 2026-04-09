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
    m_selectedClip = -1;
    m_clipPlaying = false;
    m_clipCurrentStep = 0;
    m_clipElapsedMs = 0.0f;
    m_zoom = -1.0f; // trigger auto-fit on next draw
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

    m_clips.clear();
    int const clipCount = m_spriteSheet->GetClipCount();
    m_clips.reserve(static_cast<size_t>(clipCount));
    for (int i = 0; i < clipCount; ++i) {
        moth_graphics::graphics::SpriteSheet::ClipEntry entry;
        entry.name = m_spriteSheet->GetClipName(i);
        m_spriteSheet->GetClipDesc(entry.name, entry.desc);
        m_clips.push_back(std::move(entry));
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

    float const imgW = static_cast<float>(image->GetWidth());
    float const imgH = static_cast<float>(image->GetHeight());

    // Auto-fit on first draw after a load
    ImVec2 const totalAvail = ImGui::GetContentRegionAvail();
    if (m_zoom < 0.0f) {
        float const fitH = totalAvail.y - ImGui::GetFrameHeightWithSpacing();
        if (imgW > 0.0f && imgH > 0.0f && totalAvail.x > 0.0f && fitH > 0.0f) {
            m_zoom = std::min(totalAvail.x / imgW, fitH / imgH);
        } else {
            m_zoom = 1.0f;
        }
    }

    // Toolbar
    if (ImGui::Button("Fit")) {
        float const fitH = totalAvail.y - ImGui::GetFrameHeightWithSpacing();
        if (imgW > 0.0f && imgH > 0.0f && totalAvail.x > 0.0f && fitH > 0.0f) {
            m_zoom = std::min(totalAvail.x / imgW, fitH / imgH);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("1:1")) {
        m_zoom = 1.0f;
    }
    ImGui::SameLine();
    ImGui::Text("%.0f%%", m_zoom * 100.0f);

    // Scrollable viewport
    ImVec2 const childSize = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##preview_scroll", childSize, ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Mouse-wheel zoom centered on cursor
    if (ImGui::IsWindowHovered()) {
        float const wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            ImVec2 const mouse   = ImGui::GetMousePos();
            ImVec2 const winPos  = ImGui::GetWindowPos();
            float const mouseRelX = (mouse.x - winPos.x) + ImGui::GetScrollX();
            float const mouseRelY = (mouse.y - winPos.y) + ImGui::GetScrollY();
            float const imgSpaceX = mouseRelX / m_zoom;
            float const imgSpaceY = mouseRelY / m_zoom;

            float const factor = (wheel > 0.0f) ? 1.1f : (1.0f / 1.1f);
            m_zoom = std::clamp(m_zoom * factor, 0.05f, 32.0f);

            ImGui::SetScrollX((imgSpaceX * m_zoom) - (mouse.x - winPos.x));
            ImGui::SetScrollY((imgSpaceY * m_zoom) - (mouse.y - winPos.y));
        }
    }

    float const displayW = imgW * m_zoom;
    float const displayH = imgH * m_zoom;

    ImVec2 const imagePos = ImGui::GetCursorScreenPos();
    image->ImGui({ static_cast<int>(displayW), static_cast<int>(displayH) });

    // Click on the image to select the frame under the cursor
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ImVec2 const mouse = ImGui::GetMousePos();
        float const relX = (mouse.x - imagePos.x) / m_zoom;
        float const relY = (mouse.y - imagePos.y) / m_zoom;

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

    // Frame rect overlays and pivot crosses — drawn with child's draw list so they clip to the scroll viewport
    ImDrawList* const drawList = ImGui::GetWindowDrawList();
    ImU32 const normalU32   = ImGui::ColorConvertFloat4ToU32(m_normalColor);
    ImU32 const selectedU32 = ImGui::ColorConvertFloat4ToU32(m_selectedColor);
    for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
        auto const& fr = m_frames[i];
        bool const selected = (i == m_selectedFrame);
        ImU32 const color     = selected ? selectedU32 : normalU32;
        float const thickness = selected ? 2.0f : 1.5f;

        float const x0 = imagePos.x + (static_cast<float>(fr.rect.x())      * m_zoom);
        float const y0 = imagePos.y + (static_cast<float>(fr.rect.y())      * m_zoom);
        float const x1 = imagePos.x + (static_cast<float>(fr.rect.right())  * m_zoom);
        float const y1 = imagePos.y + (static_cast<float>(fr.rect.bottom()) * m_zoom);
        drawList->AddRect({ x0, y0 }, { x1, y1 }, color, 0.0f, 0, thickness);

        // Pivot cross — pivot is relative to frame top-left, fixed 5px arm length
        float const px = imagePos.x + (static_cast<float>(fr.rect.x() + fr.pivot.x) * m_zoom);
        float const py = imagePos.y + (static_cast<float>(fr.rect.y() + fr.pivot.y) * m_zoom);
        constexpr float kArm = 5.0f;
        drawList->AddLine({ px - kArm, py }, { px + kArm, py }, color, thickness);
        drawList->AddLine({ px, py - kArm }, { px, py + kArm }, color, thickness);
    }

    ImGui::EndChild();
}

void SpriteEditor::DrawDataEditor() {
    // Overlay color pickers
    ImGui::ColorEdit4("Normal##rcol",   &m_normalColor.x,   ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine(); ImGui::TextUnformatted("Normal");
    ImGui::SameLine(0.0f, 16.0f);
    ImGui::ColorEdit4("Selected##rcol", &m_selectedColor.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine(); ImGui::TextUnformatted("Selected");

    ImGui::Separator();

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
            int pivotX = fr.pivot.x;
            int pivotY = fr.pivot.y;

            ImGui::SeparatorText(fmt::format("Frame {}", m_selectedFrame).c_str());

            // 4-column table: label | input | label | input
            // Fixed-width label columns prevent labels from overflowing the right edge.
            if (ImGui::BeginTable("##fedit_tbl", 4, ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupColumn("##fl1", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("##fv1", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##fl2", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("##fv2", ImGuiTableColumnFlags_WidthStretch);

                auto editRow = [](char const* l1, char const* id1, int& v1,
                                  char const* l2, char const* id2, int& v2) -> std::pair<bool, bool> {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(l1);
                    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN); bool const c1 = ImGui::InputInt(id1, &v1);
                    ImGui::TableSetColumnIndex(2); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(l2);
                    ImGui::TableSetColumnIndex(3); ImGui::SetNextItemWidth(-FLT_MIN); bool const c2 = ImGui::InputInt(id2, &v2);
                    return { c1, c2 };
                };

                auto const [xChanged, yChanged] = editRow("X", "##fedit_x", x, "Y", "##fedit_y", y);
                auto const [wChanged, hChanged] = editRow("W", "##fedit_w", w, "H", "##fedit_h", h);
                auto const [pxChanged, pyChanged] = editRow("Pivot X", "##fedit_px", pivotX, "Pivot Y", "##fedit_py", pivotY);

                ImGui::EndTable();

                if (xChanged || yChanged || wChanged || hChanged) {
                    w = std::max(w, 1);
                    h = std::max(h, 1);
                    fr.rect = moth_graphics::MakeRect(x, y, w, h);
                }
                if (pxChanged) { fr.pivot.x = pivotX; }
                if (pyChanged) { fr.pivot.y = pivotY; }
            }
        }
    }

    ImGui::Separator();

    // ---- Clip list (editable) ----
    ImGui::Text("Clips: %d", static_cast<int>(m_clips.size()));

    // ---- Clip preview ----
    if (m_selectedClip >= 0 && m_selectedClip < static_cast<int>(m_clips.size())) {
        auto const& clip = m_clips[m_selectedClip];

        // Advance animation
        if (m_clipPlaying && !clip.desc.frames.empty()) {
            m_clipElapsedMs += ImGui::GetIO().DeltaTime * 1000.0f;
            while (m_clipPlaying) {
                int const dur = std::max(clip.desc.frames[m_clipCurrentStep].durationMs, 1);
                if (m_clipElapsedMs < static_cast<float>(dur)) {
                    break;
                }
                m_clipElapsedMs -= static_cast<float>(dur);
                int const next = m_clipCurrentStep + 1;
                if (next >= static_cast<int>(clip.desc.frames.size())) {
                    switch (clip.desc.loop) {
                    case moth_graphics::graphics::SpriteSheet::LoopType::Stop:
                        m_clipCurrentStep = static_cast<int>(clip.desc.frames.size()) - 1;
                        m_clipPlaying = false;
                        m_clipElapsedMs = 0.0f;
                        break;
                    case moth_graphics::graphics::SpriteSheet::LoopType::Reset:
                        m_clipCurrentStep = 0;
                        m_clipPlaying = false;
                        m_clipElapsedMs = 0.0f;
                        break;
                    case moth_graphics::graphics::SpriteSheet::LoopType::Loop:
                        m_clipCurrentStep = 0;
                        break;
                    }
                } else {
                    m_clipCurrentStep = next;
                }
            }
        }

        // Draw the current frame
        auto const* image = m_spriteSheet->GetImage().get();
        if (image != nullptr && !clip.desc.frames.empty()) {
            m_clipCurrentStep = std::clamp(m_clipCurrentStep, 0, static_cast<int>(clip.desc.frames.size()) - 1);
            int const frameIdx = clip.desc.frames[m_clipCurrentStep].frameIndex;
            if (frameIdx >= 0 && frameIdx < static_cast<int>(m_frames.size())) {
                auto const& fr = m_frames[frameIdx];
                float const imgW = static_cast<float>(image->GetWidth());
                float const imgH = static_cast<float>(image->GetHeight());

                moth_graphics::FloatVec2 const uv0{
                    static_cast<float>(fr.rect.x())     / imgW,
                    static_cast<float>(fr.rect.y())     / imgH };
                moth_graphics::FloatVec2 const uv1{
                    static_cast<float>(fr.rect.right())  / imgW,
                    static_cast<float>(fr.rect.bottom()) / imgH };

                constexpr float kPreviewH = 120.0f;
                float const aspect = (fr.rect.h() > 0)
                    ? (static_cast<float>(fr.rect.w()) / static_cast<float>(fr.rect.h()))
                    : 1.0f;
                float const availW = ImGui::GetContentRegionAvail().x;
                float dispW = kPreviewH * aspect;
                float dispH = kPreviewH;
                if (dispW > availW) {
                    dispW = availW;
                    dispH = (aspect > 0.0f) ? (availW / aspect) : kPreviewH;
                }

                float const offsetX = (availW - dispW) * 0.5f;
                if (offsetX > 0.0f) {
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                }
                image->ImGui({ static_cast<int>(dispW), static_cast<int>(dispH) }, uv0, uv1);
            }
        } else if (clip.desc.frames.empty()) {
            ImGui::TextDisabled("(no steps)");
        }

        // Playback controls
        if (ImGui::Button(m_clipPlaying ? "Stop" : "Play")) {
            if (m_clipPlaying) {
                m_clipPlaying = false;
            } else {
                m_clipCurrentStep = 0;
                m_clipElapsedMs = 0.0f;
                m_clipPlaying = true;
            }
        }
        ImGui::SameLine();
        int const totalSteps = static_cast<int>(clip.desc.frames.size());
        if (ImGui::Button("Step") && totalSteps > 0) {
            m_clipPlaying = false;
            m_clipElapsedMs = 0.0f;
            int const next = m_clipCurrentStep + 1;
            if (next >= totalSteps) {
                switch (clip.desc.loop) {
                case moth_graphics::graphics::SpriteSheet::LoopType::Stop:
                    m_clipCurrentStep = totalSteps - 1;
                    break;
                case moth_graphics::graphics::SpriteSheet::LoopType::Reset:
                case moth_graphics::graphics::SpriteSheet::LoopType::Loop:
                    m_clipCurrentStep = 0;
                    break;
                }
            } else {
                m_clipCurrentStep = next;
            }
        }
        ImGui::SameLine();
        ImGui::Text("Step %d / %d  \"%s\"", totalSteps > 0 ? m_clipCurrentStep + 1 : 0, totalSteps, clip.name.c_str());
    }

    // New-clip row
    {
        float const addBtnW = ImGui::CalcTextSize("+ Clip").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - addBtnW - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputText("##new_clip_name", m_newClipNameBuffer, sizeof(m_newClipNameBuffer) - 1);
        ImGui::SameLine();
        if (ImGui::Button("+ Clip")) {
            if (m_newClipNameBuffer[0] != '\0') {
                moth_graphics::graphics::SpriteSheet::ClipEntry newClip;
                newClip.name = m_newClipNameBuffer;
                newClip.desc.loop = moth_graphics::graphics::SpriteSheet::LoopType::Stop;
                m_clips.push_back(std::move(newClip));
                m_newClipNameBuffer[0] = '\0';
            }
        }
    }

    if (ImGui::CollapsingHeader("Clips", ImGuiTreeNodeFlags_DefaultOpen)) {
        int clipToDelete = -1;

        for (int c = 0; c < static_cast<int>(m_clips.size()); ++c) {
            auto& clip = m_clips[c];
            ImGui::PushID(c);

            // Tree node with stable ID so rename doesn't collapse it
            std::string const nodeLabel = fmt::format("{} ({} steps)###clip_{}", clip.name, clip.desc.frames.size(), c);
            bool const open = ImGui::TreeNode(nodeLabel.c_str());
            if (ImGui::IsItemClicked()) {
                m_selectedClip = c;
                m_clipCurrentStep = 0;
                m_clipElapsedMs = 0.0f;
                m_clipPlaying = false;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                clipToDelete = c;
            }

            if (open) {
                // Name
                char nameBuf[256];
                strncpy(nameBuf, clip.name.c_str(), sizeof(nameBuf) - 1);
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##cname", nameBuf, sizeof(nameBuf))) {
                    clip.name = nameBuf;
                }

                // Loop type
                static char const* const kLoopItems[] = { "Stop", "Reset", "Loop" };
                int loopIdx = static_cast<int>(clip.desc.loop);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##loop", &loopIdx, kLoopItems, 3)) {
                    clip.desc.loop = static_cast<moth_graphics::graphics::SpriteSheet::LoopType>(loopIdx);
                }

                // Steps table: # | Frame | ms | [X]
                int stepToDelete = -1;
                int const maxFrameIdx = static_cast<int>(m_frames.size()) - 1;

                if (ImGui::BeginTable("##steps", 4,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
                    ImGui::TableSetupColumn("#",     ImGuiTableColumnFlags_WidthFixed,  25.0f);
                    ImGui::TableSetupColumn("Frame", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("ms",    ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("",      ImGuiTableColumnFlags_WidthFixed,  22.0f);
                    ImGui::TableHeadersRow();

                    for (int f = 0; f < static_cast<int>(clip.desc.frames.size()); ++f) {
                        auto& step = clip.desc.frames[f];
                        ImGui::TableNextRow();
                        ImGui::PushID(f);

                        ImGui::TableSetColumnIndex(0);
                        bool const stepSelected = (m_selectedFrame == step.frameIndex);
                        if (ImGui::Selectable(fmt::format("{}", f).c_str(), stepSelected,
                                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                                ImVec2(0, 0))) {
                            m_selectedFrame = step.frameIndex;
                        }

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputInt("##fi", &step.frameIndex, 0, 0)) {
                            step.frameIndex = std::clamp(step.frameIndex, 0, std::max(maxFrameIdx, 0));
                        }

                        ImGui::TableSetColumnIndex(2);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputInt("##ms", &step.durationMs, 0, 0)) {
                            step.durationMs = std::max(step.durationMs, 0);
                        }

                        ImGui::TableSetColumnIndex(3);
                        if (ImGui::SmallButton("X")) {
                            stepToDelete = f;
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }

                if (stepToDelete >= 0) {
                    clip.desc.frames.erase(clip.desc.frames.begin() + stepToDelete);
                }

                // Add step — defaults to the currently selected frame (or 0)
                if (ImGui::Button("+ Step")) {
                    moth_graphics::graphics::SpriteSheet::ClipFrame newStep;
                    newStep.frameIndex = (m_selectedFrame >= 0) ? m_selectedFrame : 0;
                    newStep.durationMs = clip.desc.frames.empty() ? 100 : clip.desc.frames.back().durationMs;
                    clip.desc.frames.push_back(newStep);
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (clipToDelete >= 0) {
            m_clips.erase(m_clips.begin() + clipToDelete);
            if (m_selectedClip == clipToDelete) {
                m_selectedClip = -1;
                m_clipPlaying = false;
            } else if (m_selectedClip > clipToDelete) {
                m_selectedClip--;
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
