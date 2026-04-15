#include "common.h"
#include "sprite_editor.h"

void SpriteEditor::DrawClipsPane() {
    ImGui::Text("Clips: %d", static_cast<int>(m_clips.size()));

    // ---- Clip preview ----
    if (m_selectedClip >= 0 && m_selectedClip < static_cast<int>(m_clips.size())) {
        auto const& clip = m_clips[m_selectedClip];

        // Advance animation playback
        if (m_clipPlaying && !clip.desc.frames.empty()) {
            m_clipElapsedMs += ImGui::GetIO().DeltaTime * 1000.0f;
            m_clipCurrentStep = std::clamp(m_clipCurrentStep, 0,
                static_cast<int>(clip.desc.frames.size()) - 1);
            while (m_clipPlaying) {
                int const dur = std::max(clip.desc.frames[m_clipCurrentStep].durationMs, 1);
                if (m_clipElapsedMs < static_cast<float>(dur)) { break; }
                m_clipElapsedMs -= static_cast<float>(dur);
                int const next = m_clipCurrentStep + 1;
                if (next < static_cast<int>(clip.desc.frames.size())) {
                    m_clipCurrentStep = next;
                } else {
                    using LoopType = moth_graphics::graphics::SpriteSheet::LoopType;
                    switch (clip.desc.loop) {
                    case LoopType::Stop:
                        m_clipCurrentStep = static_cast<int>(clip.desc.frames.size()) - 1;
                        m_clipPlaying = false;
                        m_clipElapsedMs = 0.0f;
                        break;
                    case LoopType::Reset:
                        m_clipCurrentStep = 0;
                        m_clipPlaying = false;
                        m_clipElapsedMs = 0.0f;
                        break;
                    case LoopType::Loop:
                        m_clipCurrentStep = 0;
                        break;
                    }
                }
            }
        }

        // Draw the current frame, pivot-anchored
        auto const* image = m_spriteSheet->GetImage().get();
        if (clip.desc.frames.empty()) {
            ImGui::TextDisabled("(no steps)");
        } else if (image != nullptr) {
            m_clipCurrentStep = std::clamp(m_clipCurrentStep, 0,
                static_cast<int>(clip.desc.frames.size()) - 1);
            int const maxFrameIdx = static_cast<int>(m_frames.size()) - 1;

            // Compute bounding box in pivot-relative space so the anchor stays fixed
            // across all frames in the clip.
            int minOX = 0;
            int maxOX = 1;
            int minOY = 0;
            int maxOY = 1;
            if (maxFrameIdx >= 0) {
                auto const expand = [&](moth_graphics::graphics::SpriteSheet::FrameEntry const& f) {
                    minOX = std::min(minOX, -f.pivot.x);
                    maxOX = std::max(maxOX, f.rect.w() - f.pivot.x);
                    minOY = std::min(minOY, -f.pivot.y);
                    maxOY = std::max(maxOY, f.rect.h() - f.pivot.y);
                };
                // Initialise from first step then expand over the rest
                {
                    auto const& f0 = m_frames[std::clamp(clip.desc.frames[0].frameIndex, 0, maxFrameIdx)];
                    minOX = -f0.pivot.x;  maxOX = f0.rect.w() - f0.pivot.x;
                    minOY = -f0.pivot.y;  maxOY = f0.rect.h() - f0.pivot.y;
                }
                for (int si = 1; si < static_cast<int>(clip.desc.frames.size()); ++si) {
                    expand(m_frames[std::clamp(clip.desc.frames[si].frameIndex, 0, maxFrameIdx)]);
                }
            }
            int const boundW = std::max(maxOX - minOX, 1);
            int const boundH = std::max(maxOY - minOY, 1);

            constexpr float kPreviewH = 120.0f;
            float const availW = ImGui::GetContentRegionAvail().x;
            float const zoom = std::min(availW / static_cast<float>(boundW),
                                        kPreviewH / static_cast<float>(boundH));

            if (ImGui::BeginChild("##clip_canvas", ImVec2{ availW, kPreviewH }, ImGuiChildFlags_None,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                ImVec2 const canvasMin = ImGui::GetWindowPos();
                ImVec2 const canvasMax{ canvasMin.x + availW, canvasMin.y + kPreviewH };

                float const contentW = static_cast<float>(boundW) * zoom;
                float const contentH = static_cast<float>(boundH) * zoom;
                float const anchorX  = canvasMin.x + ((availW    - contentW) * 0.5f) + (static_cast<float>(-minOX) * zoom);
                float const anchorY  = canvasMin.y + ((kPreviewH - contentH) * 0.5f) + (static_cast<float>(-minOY) * zoom);

                ImDrawList* const dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(canvasMin, canvasMax, IM_COL32(30, 30, 30, 200));

                // Draw current frame with its pivot landing on the anchor point
                int const frameIdx = clip.desc.frames[m_clipCurrentStep].frameIndex;
                if (frameIdx >= 0 && frameIdx <= maxFrameIdx) {
                    auto const& fr = m_frames[frameIdx];
                    float const imgW = static_cast<float>(image->GetWidth());
                    float const imgH = static_cast<float>(image->GetHeight());
                    moth_graphics::FloatVec2 const uv0{
                        static_cast<float>(fr.rect.x())     / imgW,
                        static_cast<float>(fr.rect.y())     / imgH };
                    moth_graphics::FloatVec2 const uv1{
                        static_cast<float>(fr.rect.right()) / imgW,
                        static_cast<float>(fr.rect.bottom())/ imgH };
                    float const fw = static_cast<float>(fr.rect.w()) * zoom;
                    float const fh = static_cast<float>(fr.rect.h()) * zoom;
                    if (fw > 0.0f && fh > 0.0f) {
                        ImGui::SetCursorScreenPos({
                            anchorX - (static_cast<float>(fr.pivot.x) * zoom),
                            anchorY - (static_cast<float>(fr.pivot.y) * zoom) });
                        image->ImGui({ static_cast<int>(fw), static_cast<int>(fh) }, uv0, uv1);
                    }
                }

                // Pivot crosshair
                constexpr float kArm = 5.0f;
                dl->AddLine({ anchorX - kArm, anchorY }, { anchorX + kArm, anchorY }, IM_COL32(255, 80, 80, 220), 1.5f);
                dl->AddLine({ anchorX, anchorY - kArm }, { anchorX, anchorY + kArm }, IM_COL32(255, 80, 80, 220), 1.5f);
            }
            ImGui::EndChild();
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
            if (next < totalSteps) {
                m_clipCurrentStep = next;
            } else {
                using LoopType = moth_graphics::graphics::SpriteSheet::LoopType;
                m_clipCurrentStep = (clip.desc.loop == LoopType::Stop) ? m_clipCurrentStep : 0;
            }
        }
        ImGui::SameLine();
        ImGui::Text("Step %d / %d  \"%s\"",
            totalSteps > 0 ? m_clipCurrentStep + 1 : 0, totalSteps, clip.name.c_str());
    }

    // ---- New-clip row ----
    {
        float const addBtnW = ImGui::CalcTextSize("+ Clip").x + (ImGui::GetStyle().FramePadding.x * 2.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - addBtnW - ImGui::GetStyle().ItemSpacing.x);
        ImGui::InputText("##new_clip_name", m_newClipNameBuffer, sizeof(m_newClipNameBuffer) - 1);
        ImGui::SameLine();
        if (ImGui::Button("+ Clip") && m_newClipNameBuffer[0] != '\0') {
            auto before = m_clips;
            int const beforeSel = m_selectedClip;
            moth_graphics::graphics::SpriteSheet::ClipEntry newClip;
            newClip.name = m_newClipNameBuffer;
            newClip.desc.loop = moth_graphics::graphics::SpriteSheet::LoopType::Stop;
            m_clips.push_back(std::move(newClip));
            m_newClipNameBuffer[0] = '\0';
            m_selectedClip = static_cast<int>(m_clips.size()) - 1;
            PushClipAction(std::move(before), beforeSel, m_selectedClip);
        }
    }

    // ---- Clip list ----
    if (ImGui::CollapsingHeader("Clips", ImGuiTreeNodeFlags_DefaultOpen)) {
        int clipToDelete = -1;

        for (int c = 0; c < static_cast<int>(m_clips.size()); ++c) {
            auto& clip = m_clips[c];
            ImGui::PushID(c);

            // Tree node with stable ID (###) so rename doesn't collapse it
            std::string const nodeLabel = fmt::format("{} ({} steps)###clip_{}", clip.name, clip.desc.frames.size(), c);
            bool const open = ImGui::TreeNode(nodeLabel.c_str());
            if (ImGui::IsItemClicked()) {
                m_selectedClip = c;
                m_clipCurrentStep = 0;
                m_clipElapsedMs = 0.0f;
                m_clipPlaying = false;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) { clipToDelete = c; }

            if (open) {
                // Name — deferred snapshot: capture on activate, push on deactivate after edit
                char nameBuf[256];
                strncpy(nameBuf, clip.name.c_str(), sizeof(nameBuf) - 1);
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::InputText("##cname", nameBuf, sizeof(nameBuf))) {
                    clip.name = nameBuf;
                }
                if (ImGui::IsItemActivated() && !m_pendingClipSnapshot.has_value()) {
                    m_pendingClipSnapshot = m_clips;
                }
                if (ImGui::IsItemDeactivatedAfterEdit() && m_pendingClipSnapshot.has_value()) {
                    PushClipAction(std::move(*m_pendingClipSnapshot), m_selectedClip, m_selectedClip);
                    m_pendingClipSnapshot.reset();
                }

                // Loop type (immediate)
                static char const* const kLoopItems[] = { "Stop", "Reset", "Loop" };
                int loopIdx = static_cast<int>(clip.desc.loop);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##loop", &loopIdx, kLoopItems, 3)) {
                    auto before = m_clips;
                    clip.desc.loop = static_cast<moth_graphics::graphics::SpriteSheet::LoopType>(loopIdx);
                    PushClipAction(std::move(before), m_selectedClip, m_selectedClip);
                }

                // Steps table: # | Frame | ms | [X]
                int stepToDelete = -1;
                int const maxFrameIdx = static_cast<int>(m_frames.size()) - 1;
                bool anyActivated   = false;
                bool anyDeactivated = false;

                char stepBuf[16];
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
                        snprintf(stepBuf, sizeof(stepBuf), "%d", f);
                        if (ImGui::Selectable(stepBuf, stepSelected,
                                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                                ImVec2(0, 0))) {
                            if (!m_frames.empty()) {
                                m_selectedFrame = std::clamp(step.frameIndex, 0,
                                    static_cast<int>(m_frames.size()) - 1);
                            }
                            m_clipCurrentStep = f;
                            m_clipElapsedMs = 0.0f;
                            m_clipPlaying = false;
                        }

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputInt("##fi", &step.frameIndex, 0, 0)) {
                            step.frameIndex = std::clamp(step.frameIndex, 0, std::max(maxFrameIdx, 0));
                        }
                        anyActivated   |= ImGui::IsItemActivated();
                        anyDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

                        ImGui::TableSetColumnIndex(2);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputInt("##ms", &step.durationMs, 0, 0)) {
                            step.durationMs = std::max(step.durationMs, 0);
                        }
                        anyActivated   |= ImGui::IsItemActivated();
                        anyDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

                        ImGui::TableSetColumnIndex(3);
                        if (ImGui::SmallButton("X")) { stepToDelete = f; }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }

                if (anyActivated && !m_pendingClipSnapshot.has_value()) {
                    m_pendingClipSnapshot = m_clips;
                }
                if (anyDeactivated && m_pendingClipSnapshot.has_value()) {
                    PushClipAction(std::move(*m_pendingClipSnapshot), m_selectedClip, m_selectedClip);
                    m_pendingClipSnapshot.reset();
                }

                if (stepToDelete >= 0) {
                    auto before = m_clips;
                    clip.desc.frames.erase(clip.desc.frames.begin() + stepToDelete);
                    PushClipAction(std::move(before), m_selectedClip, m_selectedClip);
                }

                // Add step — defaults to the currently selected frame (or 0)
                if (ImGui::Button("+ Step")) {
                    auto before = m_clips;
                    moth_graphics::graphics::SpriteSheet::ClipFrame newStep;
                    newStep.frameIndex = (m_selectedFrame >= 0 && maxFrameIdx >= 0)
                        ? std::clamp(m_selectedFrame, 0, maxFrameIdx) : 0;
                    newStep.durationMs = clip.desc.frames.empty() ? 100 : clip.desc.frames.back().durationMs;
                    clip.desc.frames.push_back(newStep);
                    PushClipAction(std::move(before), m_selectedClip, m_selectedClip);
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (clipToDelete >= 0) {
            auto before = m_clips;
            int const beforeSel = m_selectedClip;
            m_clips.erase(m_clips.begin() + clipToDelete);
            if (m_selectedClip == clipToDelete) {
                m_selectedClip = -1;
                m_clipPlaying = false;
            } else if (m_selectedClip > clipToDelete) {
                --m_selectedClip;
            }
            PushClipAction(std::move(before), beforeSel, m_selectedClip);
        }
    }
}
