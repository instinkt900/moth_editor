#include "common.h"
#include "sprite_editor.h"
#include "editor/editor_layer.h"

void SpriteEditor::DrawFramesPane() {
    ImGui::Text("Frames: %d", static_cast<int>(m_frames.size()));

    if (ImGui::CollapsingHeader("Frames", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Side-by-side: frame list (left) + selected frame preview (right)
        if (ImGui::BeginTable("##frames_layout", 2, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("##fl_list", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##fl_prev", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableNextRow();

            // Frame list
            ImGui::TableSetColumnIndex(0);
            int frameToDelete = -1;
            if (ImGui::BeginTable("##frames_table", 6,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame,
                    ImVec2(0.0f, 150.0f))) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("#",      ImGuiTableColumnFlags_WidthFixed,   30.0f);
                ImGui::TableSetupColumn("X",      ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Y",      ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("W",      ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("H",      ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##fdel", ImGuiTableColumnFlags_WidthFixed,   20.0f);
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

                    ImGui::TableSetColumnIndex(1); ImGui::Text("%d", fr.rect.x());
                    ImGui::TableSetColumnIndex(2); ImGui::Text("%d", fr.rect.y());
                    ImGui::TableSetColumnIndex(3); ImGui::Text("%d", fr.rect.w());
                    ImGui::TableSetColumnIndex(4); ImGui::Text("%d", fr.rect.h());

                    ImGui::TableSetColumnIndex(5);
                    if (ImGui::SmallButton("x")) {
                        frameToDelete = i;
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }

            if (ImGui::Button("+ Frame")) {
                moth_graphics::graphics::SpriteSheet::FrameEntry newFrame;
                newFrame.rect  = moth_graphics::MakeRect(0, 0, 32, 32);
                newFrame.pivot = { 0, 0 };
                auto const before = m_frames;
                int const beforeSel = m_selectedFrame;
                m_frames.push_back(newFrame);
                m_selectedFrame = static_cast<int>(m_frames.size()) - 1;
                auto const after = m_frames;
                int const afterSel = m_selectedFrame;
                AddSpriteAction(std::make_unique<BasicAction>(
                    [this, after, afterSel]()   { m_frames = after;  m_selectedFrame = afterSel; },
                    [this, before, beforeSel]() { m_frames = before; m_selectedFrame = beforeSel; }
                ));
            }

            if (frameToDelete >= 0) {
                auto const beforeFrames = m_frames;
                auto const beforeClips  = m_clips;
                int const beforeSel = m_selectedFrame;
                m_frames.erase(m_frames.begin() + frameToDelete);
                // Fix up selection
                if (m_selectedFrame == frameToDelete) {
                    m_selectedFrame = -1;
                } else if (m_selectedFrame > frameToDelete) {
                    m_selectedFrame--;
                }
                // Fix up any clip steps that referenced this frame
                for (auto& clip : m_clips) {
                    for (auto& step : clip.desc.frames) {
                        if (step.frameIndex >= frameToDelete && step.frameIndex > 0) {
                            step.frameIndex--;
                        }
                    }
                }
                auto const afterFrames = m_frames;
                auto const afterClips  = m_clips;
                int const afterSel = m_selectedFrame;
                AddSpriteAction(std::make_unique<BasicAction>(
                    [this, afterFrames, afterClips, afterSel]() {
                        m_frames = afterFrames; m_clips = afterClips; m_selectedFrame = afterSel;
                    },
                    [this, beforeFrames, beforeClips, beforeSel]() {
                        m_frames = beforeFrames; m_clips = beforeClips; m_selectedFrame = beforeSel;
                    }
                ));
            }

            // Selected frame mini-preview (with pivot drag)
            ImGui::TableSetColumnIndex(1);
            auto const* image = m_spriteSheet->GetImage().get();
            if (image != nullptr && m_selectedFrame >= 0 && m_selectedFrame < static_cast<int>(m_frames.size())) {
                auto& fr = m_frames[m_selectedFrame];
                float const imgW = static_cast<float>(image->GetWidth());
                float const imgH = static_cast<float>(image->GetHeight());
                // Clamp UVs so the preview shows the clamped region even when the
                // frame rect extends beyond the imported image.
                moth_graphics::FloatVec2 const uv0{
                    std::clamp(static_cast<float>(fr.rect.x())      / imgW, 0.0f, 1.0f),
                    std::clamp(static_cast<float>(fr.rect.y())      / imgH, 0.0f, 1.0f) };
                moth_graphics::FloatVec2 const uv1{
                    std::clamp(static_cast<float>(fr.rect.right())  / imgW, 0.0f, 1.0f),
                    std::clamp(static_cast<float>(fr.rect.bottom()) / imgH, 0.0f, 1.0f) };
                constexpr float kMaxH = 150.0f;
                float const col = ImGui::GetContentRegionAvail().x;
                float const aspect = (fr.rect.h() > 0)
                    ? (static_cast<float>(fr.rect.w()) / static_cast<float>(fr.rect.h()))
                    : 1.0f;
                float dispW = col;
                float dispH = (aspect > 0.0f) ? (col / aspect) : kMaxH;
                if (dispH > kMaxH) {
                    dispH = kMaxH;
                    dispW = kMaxH * aspect;
                }
                dispW = std::max(dispW, 1.0f);
                dispH = std::max(dispH, 1.0f);

                float const scaleX = (fr.rect.w() > 0) ? (dispW / static_cast<float>(fr.rect.w())) : 1.0f;
                float const scaleY = (fr.rect.h() > 0) ? (dispH / static_cast<float>(fr.rect.h())) : 1.0f;

                // Use a child window so cursor tracking is fully isolated from the
                // parent table — eliminates the InvisibleButton+SetCursorScreenPos
                // pattern that corrupted layout when frame rects were outside the image.
                if (ImGui::BeginChild("##fp_child", ImVec2{ dispW, dispH }, ImGuiChildFlags_None,
                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                    ImVec2 const childMin = ImGui::GetWindowPos();

                    image->ImGui({ static_cast<int>(dispW), static_cast<int>(dispH) }, uv0, uv1);

                    // Overlay an InvisibleButton across the full child area so ImGui
                    // claims the left-button press and the parent window cannot drag.
                    // SetCursorScreenPos is safe here because we are already inside the
                    // isolated ##fp_child — it cannot corrupt the parent table layout.
                    ImGui::SetCursorScreenPos(childMin);
                    ImGui::InvisibleButton("##pivot_drag_area", ImVec2{ dispW, dispH });

                    if (ImGui::IsItemActivated()) {
                        m_pivotDragSnapshot = m_frames;
                        m_pivotDragging = true;
                    }
                    if (ImGui::IsItemActive()) {
                        ImVec2 const mouse = ImGui::GetMousePos();
                        float const relX = (mouse.x - childMin.x) / scaleX;
                        float const relY = (mouse.y - childMin.y) / scaleY;
                        fr.pivot.x = static_cast<int>(std::round(std::clamp(relX, 0.0f, static_cast<float>(fr.rect.w()))));
                        fr.pivot.y = static_cast<int>(std::round(std::clamp(relY, 0.0f, static_cast<float>(fr.rect.h()))));
                    }
                    if (ImGui::IsItemDeactivated() && m_pivotDragging) {
                        if (m_pivotDragSnapshot.has_value()) {
                            bool changed = false;
                            if (m_selectedFrame >= 0 &&
                                m_selectedFrame < static_cast<int>(m_frames.size()) &&
                                m_selectedFrame < static_cast<int>(m_pivotDragSnapshot->size())) {
                                auto const& oldFr = (*m_pivotDragSnapshot)[m_selectedFrame];
                                changed = (oldFr.pivot.x != fr.pivot.x || oldFr.pivot.y != fr.pivot.y);
                            }
                            if (changed) {
                                auto before = std::move(*m_pivotDragSnapshot);
                                auto const after = m_frames;
                                AddSpriteAction(std::make_unique<BasicAction>(
                                    [this, after]()  { m_frames = after; },
                                    [this, before]() { m_frames = before; }
                                ));
                            }
                            m_pivotDragSnapshot.reset();
                        }
                        m_pivotDragging = false;
                    }

                    // Pivot crosshair
                    float const px = childMin.x + (static_cast<float>(fr.pivot.x) * scaleX);
                    float const py = childMin.y + (static_cast<float>(fr.pivot.y) * scaleY);
                    ImDrawList* const dl = ImGui::GetWindowDrawList();
                    constexpr float kArm = 5.0f;
                    auto const& selCol = m_editorLayer.GetConfig().SpriteEditorSelectedColor;
                    ImU32 const crossColor = ImGui::ColorConvertFloat4ToU32(
                        ImVec4{ selCol.data[0], selCol.data[1], selCol.data[2], selCol.data[3] });
                    dl->AddLine({ px - kArm, py }, { px + kArm, py }, crossColor, 1.5f);
                    dl->AddLine({ px, py - kArm }, { px, py + kArm }, crossColor, 1.5f);
                }
                ImGui::EndChild();
            } else {
                ImGui::TextDisabled("--");
            }

            ImGui::EndTable();
        }

        // ---- Frame editor (InputInt fields + pivot preset grid) ----
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

                bool anyFrameActivated   = false;
                bool anyFrameDeactivated = false;
                auto editRow = [&anyFrameActivated, &anyFrameDeactivated](
                        char const* l1, char const* id1, int& v1,
                        char const* l2, char const* id2, int& v2) -> std::pair<bool, bool> {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(l1);
                    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-FLT_MIN); bool const c1 = ImGui::InputInt(id1, &v1);
                    anyFrameActivated   |= ImGui::IsItemActivated();
                    anyFrameDeactivated |= ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::TableSetColumnIndex(2); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(l2);
                    ImGui::TableSetColumnIndex(3); ImGui::SetNextItemWidth(-FLT_MIN); bool const c2 = ImGui::InputInt(id2, &v2);
                    anyFrameActivated   |= ImGui::IsItemActivated();
                    anyFrameDeactivated |= ImGui::IsItemDeactivatedAfterEdit();
                    return { c1, c2 };
                };

                auto const [xChanged, yChanged]   = editRow("X",       "##fedit_x",  x,      "Y",       "##fedit_y",  y);
                auto const [wChanged, hChanged]   = editRow("W",       "##fedit_w",  w,      "H",       "##fedit_h",  h);
                auto const [pxChanged, pyChanged] = editRow("Pivot X", "##fedit_px", pivotX, "Pivot Y", "##fedit_py", pivotY);

                ImGui::EndTable();

                if (xChanged || yChanged || wChanged || hChanged) {
                    w = std::max(w, 1);
                    h = std::max(h, 1);
                    fr.rect = moth_graphics::MakeRect(x, y, w, h);
                }
                if (pxChanged) { fr.pivot.x = pivotX; }
                if (pyChanged) { fr.pivot.y = pivotY; }

                // On first activation, capture snapshot (after the editRow calls set the flag)
                if (anyFrameActivated && !m_pendingFrameSnapshot.has_value()) {
                    m_pendingFrameSnapshot = m_frames;
                }
                // On deactivation, commit the action
                if (anyFrameDeactivated && m_pendingFrameSnapshot.has_value()) {
                    auto before = std::move(*m_pendingFrameSnapshot);
                    m_pendingFrameSnapshot.reset();
                    auto const after = m_frames;
                    int const selFrame = m_selectedFrame;
                    AddSpriteAction(std::make_unique<BasicAction>(
                        [this, after, selFrame]()  { m_frames = after;  m_selectedFrame = selFrame; },
                        [this, before, selFrame]() { m_frames = before; m_selectedFrame = selFrame; }
                    ));
                }
            }

            // Pivot preset grid
            {
                int const hw = fr.rect.w() / 2;
                int const hh = fr.rect.h() / 2;
                int const fw = fr.rect.w();
                int const fh = fr.rect.h();
                float const btnW = (ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x * 2.0f)) / 3.0f;
                ImVec2 const bs{ btnW, 0.0f };

                auto pivotPreset = [&](char const* label, ImVec2 btnSz, int px, int py) {
                    if (ImGui::Button(label, btnSz)) {
                        auto const before = m_frames;
                        fr.pivot.x = px;
                        fr.pivot.y = py;
                        auto const after = m_frames;
                        int const sel = m_selectedFrame;
                        AddSpriteAction(std::make_unique<BasicAction>(
                            [this, after, sel]()  { m_frames = after;  m_selectedFrame = sel; },
                            [this, before, sel]() { m_frames = before; m_selectedFrame = sel; }
                        ));
                    }
                };

                pivotPreset("TL##pv", bs, 0,  0);   ImGui::SameLine();
                pivotPreset("T##pv",  bs, hw, 0);   ImGui::SameLine();
                pivotPreset("TR##pv", bs, fw, 0);
                pivotPreset("L##pv",  bs, 0,  hh);  ImGui::SameLine();
                pivotPreset("C##pv",  bs, hw, hh);  ImGui::SameLine();
                pivotPreset("R##pv",  bs, fw, hh);
                pivotPreset("BL##pv", bs, 0,  fh);  ImGui::SameLine();
                pivotPreset("B##pv",  bs, hw, fh);  ImGui::SameLine();
                pivotPreset("BR##pv", bs, fw, fh);
            }
        }
    }
}
