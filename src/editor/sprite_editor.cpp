#include "common.h"
#include "sprite_editor.h"
#include "editor_layer.h"

#include "moth_graphics/graphics/igraphics.h"
#include "moth_graphics/graphics/surface_context.h"
#include "moth_graphics/graphics/asset_context.h"
#include "moth_graphics/graphics/spritesheet_factory.h"

#include <nfd.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

SpriteEditor::SpriteEditor(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

void SpriteEditor::AddSpriteAction(std::unique_ptr<IEditorAction> action) {
    while (static_cast<int>(m_undoStack.size()) - 1 > m_undoIndex) {
        m_undoStack.pop_back();
    }
    m_undoStack.push_back(std::move(action));
    ++m_undoIndex;
}

void SpriteEditor::UndoSpriteAction() {
    if (m_undoIndex >= 0) {
        m_undoStack[m_undoIndex]->Undo();
        --m_undoIndex;
    }
}

void SpriteEditor::RedoSpriteAction() {
    if (m_undoIndex < static_cast<int>(m_undoStack.size()) - 1) {
        ++m_undoIndex;
        m_undoStack[m_undoIndex]->Do();
    }
}

void SpriteEditor::ClearSpriteActions() {
    m_undoStack.clear();
    m_undoIndex = -1;
    m_pendingFrameSnapshot.reset();
    m_pendingClipSnapshot.reset();
    m_pivotDragging = false;
    m_pivotDragSnapshot.reset();
}

void SpriteEditor::LoadSpriteSheet(std::filesystem::path const& path) {
    ClearSpriteActions();
    m_selectedFrame = -1;
    m_selectedClip = -1;
    m_clipPlaying = false;
    m_clipCurrentStep = 0;
    m_clipElapsedMs = 0.0f;
    m_zoom = -1.0f; // trigger auto-fit on next draw
    m_frames.clear();
    m_imagePathBuffer[0] = '\0';

    auto& assetContext = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();
    m_spriteSheet = assetContext.GetSpriteSheetFactory().GetSpriteSheet(path);
    if (!m_spriteSheet) {
        spdlog::error("Failed to load sprite sheet: {}", path.string());
        return;
    }

    // Read the image path from the JSON so Import Sheet and Save know it
    try {
        std::ifstream ifile(path);
        if (ifile.is_open()) {
            nlohmann::json json;
            ifile >> json;
            if (json.contains("image") && json["image"].is_string()) {
                auto const imageAbsPath = std::filesystem::absolute(
                    path.parent_path() / json["image"].get<std::string>()).lexically_normal();
                auto const imageStr = imageAbsPath.string();
                strncpy(m_imagePathBuffer, imageStr.c_str(), sizeof(m_imagePathBuffer) - 1);
                m_imagePathBuffer[sizeof(m_imagePathBuffer) - 1] = '\0';
            }
        }
    } catch (std::exception const& e) {
        spdlog::warn("SpriteEditor: could not read image path from '{}': {}", path.string(), e.what());
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

void SpriteEditor::ImportSheet(std::filesystem::path const& imagePath) {
    auto& assetContext = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();
    auto image = assetContext.ImageFromFile(imagePath);
    if (!image) {
        spdlog::error("SpriteEditor: failed to load image '{}'", imagePath.string());
        return;
    }

    auto imageStr = imagePath.string();
    strncpy(m_imagePathBuffer, imageStr.c_str(), sizeof(m_imagePathBuffer) - 1);
    m_imagePathBuffer[sizeof(m_imagePathBuffer) - 1] = '\0';

    m_spriteSheet = std::make_shared<moth_graphics::graphics::SpriteSheet>(
        std::shared_ptr<moth_graphics::graphics::IImage>(std::move(image)),
        m_frames,
        m_clips);
    m_zoom = -1.0f;    // re-fit to new image dimensions
    m_selectedFrame = -1;
    m_selectedClip  = -1;
    m_clipPlaying   = false;
    m_clipCurrentStep = 0;
    m_clipElapsedMs   = 0.0f;
}

void SpriteEditor::SaveSpriteSheet() {
    if (m_pathBuffer[0] == '\0' || !m_spriteSheet) {
        return;
    }

    std::filesystem::path const path = m_pathBuffer;

    // Read existing JSON to preserve unknown fields; start fresh if the file doesn't exist yet.
    nlohmann::json json = nlohmann::json::object();
    {
        std::ifstream ifile(path);
        if (ifile.is_open()) {
            try {
                ifile >> json;
            } catch (std::exception const& e) {
                spdlog::warn("SpriteEditor: could not parse existing '{}', overwriting: {}", path.string(), e.what());
                json = nlohmann::json::object();
            }
        }
    }

    // Update the image field if we have a known image path
    if (m_imagePathBuffer[0] != '\0') {
        std::filesystem::path const imagePath = m_imagePathBuffer;
        std::filesystem::path const relImage  = imagePath.lexically_relative(path.parent_path());
        json["image"] = relImage.string();
    }

    // Write frames
    nlohmann::json framesJson = nlohmann::json::array();
    for (auto const& fr : m_frames) {
        nlohmann::json obj;
        obj["x"]       = fr.rect.x();
        obj["y"]       = fr.rect.y();
        obj["w"]       = fr.rect.w();
        obj["h"]       = fr.rect.h();
        obj["pivot_x"] = fr.pivot.x;
        obj["pivot_y"] = fr.pivot.y;
        framesJson.push_back(std::move(obj));
    }
    json["frames"] = std::move(framesJson);

    // Write clips
    nlohmann::json clipsJson = nlohmann::json::array();
    for (auto const& entry : m_clips) {
        nlohmann::json clipObj;
        clipObj["name"] = entry.name;

        char const* loopStr = nullptr;
        switch (entry.desc.loop) {
        case moth_graphics::graphics::SpriteSheet::LoopType::Stop:  loopStr = "stop";  break;
        case moth_graphics::graphics::SpriteSheet::LoopType::Reset: loopStr = "reset"; break;
        case moth_graphics::graphics::SpriteSheet::LoopType::Loop:  loopStr = "loop";  break;
        }
        clipObj["loop"] = loopStr;

        nlohmann::json stepsJson = nlohmann::json::array();
        for (auto const& step : entry.desc.frames) {
            nlohmann::json stepObj;
            stepObj["frame"]       = step.frameIndex;
            stepObj["duration_ms"] = step.durationMs;
            stepsJson.push_back(std::move(stepObj));
        }
        clipObj["frames"] = std::move(stepsJson);

        clipsJson.push_back(std::move(clipObj));
    }
    json["clips"] = std::move(clipsJson);

    // Write to file
    std::ofstream ofile(path);
    if (!ofile.is_open()) {
        spdlog::error("SpriteEditor: failed to open '{}' for writing", path.string());
        return;
    }
    try {
        ofile << json.dump(2);
        spdlog::info("SpriteEditor: saved '{}'", path.string());
    } catch (std::exception const& e) {
        spdlog::error("SpriteEditor: failed to write '{}': {}", path.string(), e.what());
        return;
    }

    // Flush the factory cache so a subsequent load picks up the new data
    auto& assetContext = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();
    assetContext.GetSpriteSheetFactory().FlushCache();
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
    auto const& cfg = m_editorLayer.GetConfig();
    auto const toU32 = [](moth_ui::Color const& c) {
        return ImGui::ColorConvertFloat4ToU32(ImVec4{ c.data[0], c.data[1], c.data[2], c.data[3] });
    };
    ImU32 const normalU32   = toU32(cfg.SpriteEditorNormalColor);
    ImU32 const selectedU32 = toU32(cfg.SpriteEditorSelectedColor);
    for (int i = 0; i < static_cast<int>(m_frames.size()); ++i) {
        auto const& fr = m_frames[i];
        bool const selected = (i == m_selectedFrame);
        ImU32 const color     = selected ? selectedU32 : normalU32;
        float const thickness = static_cast<float>(cfg.SpriteEditorRectThickness);
        // AddRect centers its stroke on the boundary; offset by half-thickness
        // outward so the inner edge of the line exactly borders the frame.
        float const half = thickness * 0.5f;

        float const x0 = imagePos.x + (static_cast<float>(fr.rect.x())      * m_zoom) - half;
        float const y0 = imagePos.y + (static_cast<float>(fr.rect.y())      * m_zoom) - half;
        float const x1 = imagePos.x + (static_cast<float>(fr.rect.right())  * m_zoom) + half;
        float const y1 = imagePos.y + (static_cast<float>(fr.rect.bottom()) * m_zoom) + half;
        drawList->AddRect({ x0, y0 }, { x1, y1 }, color, 0.0f, 0, thickness);

        // Pivot cross — pivot is relative to frame top-left, fixed 5px arm length
        float const px = imagePos.x + (static_cast<float>(fr.rect.x() + fr.pivot.x) * m_zoom);
        float const py = imagePos.y + (static_cast<float>(fr.rect.y() + fr.pivot.y) * m_zoom);
        constexpr float kArm = 5.0f;
        drawList->AddLine({ px - kArm, py }, { px + kArm, py }, color, 1.5f);
        drawList->AddLine({ px, py - kArm }, { px, py + kArm }, color, 1.5f);
    }

    ImGui::EndChild();
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

    // ---- Frames pane (top half, independently scrollable) ----
    if (ImGui::BeginChild("##frames_pane", ImVec2(0, framesH), ImGuiChildFlags_None)) {

    // ---- Frame list ----
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
                ImGui::TableSetupColumn("#",  ImGuiTableColumnFlags_WidthFixed, 30.0f);
                ImGui::TableSetupColumn("X",  ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Y",  ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("W",  ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("H",  ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("##fdel", ImGuiTableColumnFlags_WidthFixed, 20.0f);
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

            // Selected frame preview (with pivot drag)
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
                    ImU32 const crossColor = ImGui::ColorConvertFloat4ToU32(ImVec4{ selCol.data[0], selCol.data[1], selCol.data[2], selCol.data[3] });
                    dl->AddLine({ px - kArm, py }, { px + kArm, py }, crossColor, 1.5f);
                    dl->AddLine({ px, py - kArm }, { px, py + kArm }, crossColor, 1.5f);
                }
                ImGui::EndChild();
            } else {
                ImGui::TextDisabled("--");
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

    } // end ##frames_pane
    ImGui::EndChild();

    // ---- Clips pane (bottom half, independently scrollable) ----
    ImGui::BeginChild("##clips_pane", ImVec2(0, 0), ImGuiChildFlags_None);

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

        // Draw the current frame, pivot-aligned
        auto const* image = m_spriteSheet->GetImage().get();
        if (clip.desc.frames.empty()) {
            ImGui::TextDisabled("(no steps)");
        } else if (image != nullptr) {
            m_clipCurrentStep = std::clamp(m_clipCurrentStep, 0, static_cast<int>(clip.desc.frames.size()) - 1);
            int const maxFrameIdx = static_cast<int>(m_frames.size()) - 1;

            // Bounding box in pivot-relative space: covers all steps so the
            // anchor stays fixed as frames change.
            int minOX = 0;
            int maxOX = 1;
            int minOY = 0;
            int maxOY = 1;
            bool firstStep = true;
            for (auto const& step : clip.desc.frames) {
                int const fi = std::clamp(step.frameIndex, 0, std::max(maxFrameIdx, 0));
                if (maxFrameIdx < 0) { break; }
                auto const& f = m_frames[fi];
                if (firstStep) {
                    minOX = -f.pivot.x;               maxOX = f.rect.w() - f.pivot.x;
                    minOY = -f.pivot.y;               maxOY = f.rect.h() - f.pivot.y;
                    firstStep = false;
                } else {
                    minOX = std::min(minOX, -f.pivot.x);
                    maxOX = std::max(maxOX, f.rect.w() - f.pivot.x);
                    minOY = std::min(minOY, -f.pivot.y);
                    maxOY = std::max(maxOY, f.rect.h() - f.pivot.y);
                }
            }
            int const boundW = std::max(maxOX - minOX, 1);
            int const boundH = std::max(maxOY - minOY, 1);

            constexpr float kPreviewH = 120.0f;
            float const availW = ImGui::GetContentRegionAvail().x;
            float const zoom = std::min(availW / static_cast<float>(boundW),
                                        kPreviewH / static_cast<float>(boundH));
            float const contentW = static_cast<float>(boundW) * zoom;
            float const contentH = static_cast<float>(boundH) * zoom;

            // Use a child window so its internal cursor tracking is isolated
            // from the parent — the parent always advances by exactly kPreviewH.
            if (ImGui::BeginChild("##clip_canvas", ImVec2{ availW, kPreviewH }, ImGuiChildFlags_None,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                ImVec2 const canvasMin = ImGui::GetWindowPos();
                ImVec2 const canvasMax{ canvasMin.x + availW, canvasMin.y + kPreviewH };

                // Pivot anchor in screen space (centered within canvas)
                float const anchorX = canvasMin.x + ((availW   - contentW) * 0.5f) + (static_cast<float>(-minOX) * zoom);
                float const anchorY = canvasMin.y + ((kPreviewH - contentH) * 0.5f) + (static_cast<float>(-minOY) * zoom);

                ImDrawList* const dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(canvasMin, canvasMax, IM_COL32(30, 30, 30, 200));

                // Draw current frame positioned so its pivot lands on anchorX/Y
                int const frameIdx = clip.desc.frames[m_clipCurrentStep].frameIndex;
                if (frameIdx >= 0 && frameIdx <= maxFrameIdx) {
                    auto const& fr = m_frames[frameIdx];
                    float const imgW = static_cast<float>(image->GetWidth());
                    float const imgH = static_cast<float>(image->GetHeight());

                    moth_graphics::FloatVec2 const uv0{
                        static_cast<float>(fr.rect.x())      / imgW,
                        static_cast<float>(fr.rect.y())      / imgH };
                    moth_graphics::FloatVec2 const uv1{
                        static_cast<float>(fr.rect.right())  / imgW,
                        static_cast<float>(fr.rect.bottom()) / imgH };

                    float const fx = anchorX - (static_cast<float>(fr.pivot.x) * zoom);
                    float const fy = anchorY - (static_cast<float>(fr.pivot.y) * zoom);
                    float const fw = static_cast<float>(fr.rect.w()) * zoom;
                    float const fh = static_cast<float>(fr.rect.h()) * zoom;

                    if (fw > 0.0f && fh > 0.0f) {
                        ImGui::SetCursorScreenPos({ fx, fy });
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
                auto const before = m_clips;
                int const beforeSel = m_selectedClip;
                moth_graphics::graphics::SpriteSheet::ClipEntry newClip;
                newClip.name = m_newClipNameBuffer;
                newClip.desc.loop = moth_graphics::graphics::SpriteSheet::LoopType::Stop;
                m_clips.push_back(std::move(newClip));
                m_newClipNameBuffer[0] = '\0';
                auto const after = m_clips;
                AddSpriteAction(std::make_unique<BasicAction>(
                    [this, after]()            { m_clips = after; },
                    [this, before, beforeSel]() { m_clips = before; m_selectedClip = beforeSel; }
                ));
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
                // Name (deferred: capture on activate, push on deactivate after edit)
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
                    auto before = std::move(*m_pendingClipSnapshot);
                    m_pendingClipSnapshot.reset();
                    auto const after = m_clips;
                    AddSpriteAction(std::make_unique<BasicAction>(
                        [this, after]()  { m_clips = after; },
                        [this, before]() { m_clips = before; }
                    ));
                }

                // Loop type (immediate)
                static char const* const kLoopItems[] = { "Stop", "Reset", "Loop" };
                int loopIdx = static_cast<int>(clip.desc.loop);
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::Combo("##loop", &loopIdx, kLoopItems, 3)) {
                    auto const before = m_clips;
                    clip.desc.loop = static_cast<moth_graphics::graphics::SpriteSheet::LoopType>(loopIdx);
                    auto const after = m_clips;
                    AddSpriteAction(std::make_unique<BasicAction>(
                        [this, after]()  { m_clips = after; },
                        [this, before]() { m_clips = before; }
                    ));
                }

                // Steps table: # | Frame | ms | [X]
                int stepToDelete = -1;
                int const maxFrameIdx = static_cast<int>(m_frames.size()) - 1;
                bool anyClipActivated   = false;
                bool anyClipDeactivated = false;

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
                            m_clipCurrentStep = f;
                            m_clipElapsedMs = 0.0f;
                            m_clipPlaying = false;
                        }

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputInt("##fi", &step.frameIndex, 0, 0)) {
                            step.frameIndex = std::clamp(step.frameIndex, 0, std::max(maxFrameIdx, 0));
                        }
                        anyClipActivated   |= ImGui::IsItemActivated();
                        anyClipDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

                        ImGui::TableSetColumnIndex(2);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (ImGui::InputInt("##ms", &step.durationMs, 0, 0)) {
                            step.durationMs = std::max(step.durationMs, 0);
                        }
                        anyClipActivated   |= ImGui::IsItemActivated();
                        anyClipDeactivated |= ImGui::IsItemDeactivatedAfterEdit();

                        ImGui::TableSetColumnIndex(3);
                        if (ImGui::SmallButton("X")) {
                            stepToDelete = f;
                        }

                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }

                if (anyClipActivated && !m_pendingClipSnapshot.has_value()) {
                    m_pendingClipSnapshot = m_clips;
                }
                if (anyClipDeactivated && m_pendingClipSnapshot.has_value()) {
                    auto before = std::move(*m_pendingClipSnapshot);
                    m_pendingClipSnapshot.reset();
                    auto const after = m_clips;
                    AddSpriteAction(std::make_unique<BasicAction>(
                        [this, after]()  { m_clips = after; },
                        [this, before]() { m_clips = before; }
                    ));
                }

                if (stepToDelete >= 0) {
                    auto const before = m_clips;
                    clip.desc.frames.erase(clip.desc.frames.begin() + stepToDelete);
                    auto const after = m_clips;
                    AddSpriteAction(std::make_unique<BasicAction>(
                        [this, after]()  { m_clips = after; },
                        [this, before]() { m_clips = before; }
                    ));
                }

                // Add step — defaults to the currently selected frame (or 0)
                if (ImGui::Button("+ Step")) {
                    auto const before = m_clips;
                    moth_graphics::graphics::SpriteSheet::ClipFrame newStep;
                    newStep.frameIndex = (m_selectedFrame >= 0) ? m_selectedFrame : 0;
                    newStep.durationMs = clip.desc.frames.empty() ? 100 : clip.desc.frames.back().durationMs;
                    clip.desc.frames.push_back(newStep);
                    auto const after = m_clips;
                    AddSpriteAction(std::make_unique<BasicAction>(
                        [this, after]()  { m_clips = after; },
                        [this, before]() { m_clips = before; }
                    ));
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (clipToDelete >= 0) {
            auto const before = m_clips;
            int const beforeSel = m_selectedClip;
            m_clips.erase(m_clips.begin() + clipToDelete);
            if (m_selectedClip == clipToDelete) {
                m_selectedClip = -1;
                m_clipPlaying = false;
            } else if (m_selectedClip > clipToDelete) {
                m_selectedClip--;
            }
            auto const after = m_clips;
            int const afterSel = m_selectedClip;
            AddSpriteAction(std::make_unique<BasicAction>(
                [this, after, afterSel]()   { m_clips = after;  m_selectedClip = afterSel; },
                [this, before, beforeSel]() { m_clips = before; m_selectedClip = beforeSel; }
            ));
        }
    }

    ImGui::EndChild(); // ##clips_pane
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
