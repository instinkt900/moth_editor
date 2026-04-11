#include "common.h"
#include "sprite_editor.h"
#include "editor/editor_layer.h"

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
    auto const fitZoom = [&]() {
        float const fitH = totalAvail.y - ImGui::GetFrameHeightWithSpacing();
        if (imgW > 0.0f && imgH > 0.0f && totalAvail.x > 0.0f && fitH > 0.0f) {
            m_zoom = std::min(totalAvail.x / imgW, fitH / imgH);
        }
    };
    if (m_zoom < 0.0f) {
        fitZoom();
        if (m_zoom < 0.0f) { // avail was zero — defer until next frame
            m_zoom = 1.0f;
        }
    }

    // Toolbar
    if (ImGui::Button("Fit")) {
        fitZoom();
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
