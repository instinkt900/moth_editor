#include "common.h"
#include "texture_packer.h"
#include "editor/editor_layer.h"
#include "moth_graphics/graphics/surface_context.h"
#include "moth_graphics/graphics/asset_context.h"

#include <nfd.h>

// Combo item arrays for pack options
namespace {
    char const* const kPaddingTypeItems[] = { "Color", "Extend", "Mirror", "Wrap" };
    char const* const kFormatItems[]      = { "PNG", "BMP", "TGA", "JPEG" };
    char const* const kLoopTypeItems[]    = { "Loop", "Stop", "Reset" };
}

void TexturePacker::DrawInputPanel() {
    ImGui::SeparatorText("Input Images");

    // ---- Image list ----
    float const listH = std::floor(ImGui::GetContentRegionAvail().y * 0.35f);
    if (ImGui::BeginChild("##input_list", ImVec2(0, listH),
            ImGuiChildFlags_Border, ImGuiWindowFlags_None)) {
        if (m_inputImages.empty()) {
            ImGui::TextDisabled("No images added. Use the buttons below.");
        }
        for (int i = 0; i < static_cast<int>(m_inputImages.size()); ++i) {
            auto const& img = m_inputImages[i];
            ImGui::PushID(i);

            bool const selected = (m_selectedInput == i);
            std::string const label = img.path.filename().string();
            if (ImGui::Selectable(label.c_str(), selected,
                    ImGuiSelectableFlags_AllowOverlap)) {
                if (m_selectedInput != i) {
                    m_selectedInput = i;
                    m_inputZoom = 0.0f;
                    // Load preview image
                    auto& assetContext = m_editorLayer.GetGraphics()
                        .GetSurfaceContext().GetAssetContext();
                    m_inputPreview = assetContext.ImageFromFile(img.path);
                }
            }
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 16.0f);
            if (ImGui::SmallButton("x")) {
                m_inputImages.erase(m_inputImages.begin() + i);
                if (m_selectedInput == i) {
                    m_selectedInput = -1;
                    m_inputPreview.reset();
                } else if (m_selectedInput > i) {
                    --m_selectedInput;
                }
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    // ---- Zoom toolbar (drawn before child so it reduces remaining height) ----
    bool const hasInputPreview = (m_selectedInput >= 0 &&
                                  m_selectedInput < static_cast<int>(m_inputImages.size()) &&
                                  m_inputPreview);
    if (hasInputPreview) {
        if (m_inputZoom > 0.0f) {
            ImGui::Text("%.0f%%", m_inputZoom * 100.0f);
        } else {
            ImGui::TextDisabled("Fit");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit##in")) { m_inputZoom = 0.0f; }
        ImGui::SameLine();
        if (ImGui::SmallButton("+##in")) {
            float const srcW = static_cast<float>(m_inputPreview->GetWidth());
            float const srcH = static_cast<float>(m_inputPreview->GetHeight());
            float const avW  = ImGui::GetContentRegionAvail().x;
            float const avH  = ImGui::GetContentRegionAvail().y * 0.30f;
            float fitScale = 1.0f;
            if (srcW > 0.0f && srcH > 0.0f) {
                fitScale = std::min({ avW / srcW, avH / srcH, 1.0f });
            }
            float const cur = (m_inputZoom <= 0.0f) ? fitScale : m_inputZoom;
            m_inputZoom = std::min(cur * 1.25f, 16.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("-##in")) {
            float const srcW = static_cast<float>(m_inputPreview->GetWidth());
            float const srcH = static_cast<float>(m_inputPreview->GetHeight());
            float const avW  = ImGui::GetContentRegionAvail().x;
            float const avH  = ImGui::GetContentRegionAvail().y * 0.30f;
            float fitScale = 1.0f;
            if (srcW > 0.0f && srcH > 0.0f) {
                fitScale = std::min({ avW / srcW, avH / srcH, 1.0f });
            }
            float const cur = (m_inputZoom <= 0.0f) ? fitScale : m_inputZoom;
            m_inputZoom = std::max(cur / 1.25f, 0.05f);
        }
    }

    // ---- Preview + info for selected input ----
    float const previewH = std::floor(ImGui::GetContentRegionAvail().y * 0.30f);
    if (ImGui::BeginChild("##input_preview", ImVec2(0, previewH),
            ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (hasInputPreview) {
            auto const& img = m_inputImages[m_selectedInput];
            ImGui::Text("%s", img.path.string().c_str());
            ImGui::Text("%d x %d  (%d ch)", img.dimensions.x, img.dimensions.y, img.channels);

            float const avW  = ImGui::GetContentRegionAvail().x;
            float const avH  = ImGui::GetContentRegionAvail().y;
            float const srcW = static_cast<float>(m_inputPreview->GetWidth());
            float const srcH = static_cast<float>(m_inputPreview->GetHeight());
            float fitScale = 1.0f;
            if (srcW > 0.0f && srcH > 0.0f) {
                fitScale = std::min({ avW / srcW, avH / srcH, 1.0f });
            }

            // Scroll wheel zoom
            if (ImGui::IsWindowHovered()) {
                float const wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    float const cur = (m_inputZoom <= 0.0f) ? fitScale : m_inputZoom;
                    m_inputZoom = std::clamp(cur * (wheel > 0.0f ? 1.1f : (1.0f / 1.1f)),
                                             0.05f, 16.0f);
                }
            }

            float const scale = (m_inputZoom <= 0.0f) ? fitScale : m_inputZoom;
            m_inputPreview->ImGui({
                static_cast<int>(srcW * scale),
                static_cast<int>(srcH * scale) });
        } else {
            ImGui::TextDisabled("Select an image to preview.");
        }
    }
    ImGui::EndChild();

    // ---- Add buttons ----
    ImGui::SeparatorText("Add Images");

    auto const currentPath = std::filesystem::current_path().string();

    if (ImGui::Button("Add File...")) {
        nfdchar_t* outPath = nullptr;
        if (NFD_OpenDialog("png,jpg,jpeg,bmp,tga", currentPath.c_str(), &outPath) == NFD_OKAY
                && outPath != nullptr) {
            AddFile(outPath);
            NFD_Free(outPath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Directory...")) {
        nfdchar_t* outPath = nullptr;
        if (NFD_PickFolder(currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
            AddDirectory(outPath, true);
            NFD_Free(outPath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Layout...")) {
        nfdchar_t* outPath = nullptr;
        if (NFD_OpenDialog("json", currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
            AddLayout(outPath);
            NFD_Free(outPath);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Layout Dir...")) {
        nfdchar_t* outPath = nullptr;
        if (NFD_PickFolder(currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
            AddLayoutDirectory(outPath, true);
            NFD_Free(outPath);
        }
    }

    if (ImGui::Button("Clear All") && !m_inputImages.empty()) {
        m_inputImages.clear();
        m_selectedInput = -1;
        m_inputPreview.reset();
    }

    // ---- Pack options ----
    ImGui::SeparatorText("Pack Options");

    if (ImGui::BeginTable("##pack_opts", 2, ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::LabelText("##minw_l", "Min W"); ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt("##minw", &m_minWidth, 0, 0);
        m_minWidth = std::max(m_minWidth, 1);

        ImGui::TableSetColumnIndex(1);
        ImGui::LabelText("##minh_l", "Min H"); ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt("##minh", &m_minHeight, 0, 0);
        m_minHeight = std::max(m_minHeight, 1);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::LabelText("##maxw_l", "Max W"); ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt("##maxw", &m_maxWidth, 0, 0);
        m_maxWidth = std::max(m_maxWidth, m_minWidth);

        ImGui::TableSetColumnIndex(1);
        ImGui::LabelText("##maxh_l", "Max H"); ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt("##maxh", &m_maxHeight, 0, 0);
        m_maxHeight = std::max(m_maxHeight, m_minHeight);

        ImGui::EndTable();
    }

    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Padding");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputInt("##padding", &m_padding, 0, 0);
        m_padding = std::max(m_padding, 0);
        ImGui::SameLine();
        int paddingTypeIdx = static_cast<int>(m_paddingType);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##padding_type", &paddingTypeIdx, kPaddingTypeItems, 4)) {
            m_paddingType = static_cast<moth_packer::PaddingType>(paddingTypeIdx);
        }
    }

    {
        // Background color as 8-digit hex RRGGBBAA
        char hexBuf[9];
        snprintf(hexBuf, sizeof(hexBuf), "%08X", m_paddingColor);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Background Color");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::InputText("##bg_color", hexBuf, sizeof(hexBuf),
                ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) {
            m_paddingColor = static_cast<uint32_t>(std::strtoul(hexBuf, nullptr, 16));
        }
    }

    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Format");
        ImGui::SameLine();
        int fmtIdx = static_cast<int>(m_outputFormat);
        ImGui::SetNextItemWidth(100.0f);
        if (ImGui::Combo("##format", &fmtIdx, kFormatItems, 4)) {
            m_outputFormat = static_cast<moth_packer::AtlasFormat>(fmtIdx);
        }
        if (m_outputFormat == moth_packer::AtlasFormat::JPEG) {
            ImGui::SameLine();
            ImGui::TextUnformatted("Quality");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SliderInt("##jpeg_q", &m_jpegQuality, 1, 100);
        }
    }

    // Flipbook mode toggle + options
    ImGui::Checkbox("Flipbook Mode", &m_flipbookMode);
    if (m_flipbookMode) {
        ImGui::Indent();
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("FPS");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(60.0f);
            ImGui::InputInt("##fps", &m_fps, 0, 0);
            m_fps = std::max(m_fps, 1);
        }
        {
            ImGui::SameLine();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Loop");
            ImGui::SameLine();
            int loopIdx = static_cast<int>(m_loopType);
            ImGui::SetNextItemWidth(80.0f);
            if (ImGui::Combo("##loop_type", &loopIdx, kLoopTypeItems, 3)) {
                m_loopType = static_cast<moth_packer::LoopType>(loopIdx);
            }
        }
        {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Clip Name");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::InputText("##clip_name", m_clipName, sizeof(m_clipName) - 1);
        }
        ImGui::Unindent();
    }

    ImGui::Spacing();
    bool const canPack = !m_inputImages.empty();
    if (!canPack) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Pack", ImVec2(-FLT_MIN, 0))) {
        DoPack();
    }
    if (!canPack) {
        ImGui::EndDisabled();
    }
}
