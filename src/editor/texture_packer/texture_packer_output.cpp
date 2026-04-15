#include "common.h"
#include "texture_packer.h"
#include "editor/editor_layer.h"
#include "moth_graphics/graphics/surface_context.h"
#include "moth_graphics/graphics/asset_context.h"

#include <nfd.h>

// ---- Pack ---------------------------------------------------------------

void TexturePacker::DoPack() {
    m_previewAtlases.clear();
    m_selectedOutput = -1;
    m_hasPacked = false;

    // Pack into a temp directory so we can load the atlas images for preview.
    auto const tmpDir = std::filesystem::temp_directory_path() / "moth_tp_preview";
    std::error_code ec;
    std::filesystem::create_directories(tmpDir, ec);
    if (ec) {
        auto const msg = fmt::format("TexturePacker: could not create temp dir: {}", ec.message());
        spdlog::error("{}", msg);
        m_editorLayer.ShowError(msg);
        return;
    }

    moth_packer::PackOptions opts;
    opts.outputPath    = tmpDir;
    opts.filename      = "preview";
    opts.forceOverwrite = true;
    opts.minWidth      = m_minWidth;
    opts.minHeight     = m_minHeight;
    opts.maxWidth      = m_maxWidth;
    opts.maxHeight     = m_maxHeight;
    opts.padding       = m_padding;
    opts.paddingType   = m_paddingType;
    opts.paddingColor  = m_paddingColor;
    opts.format        = m_outputFormat;
    opts.jpegQuality   = m_jpegQuality;
    opts.packType      = m_flipbookMode ? moth_packer::PackType::Flipbook
                                        : moth_packer::PackType::Atlas;
    if (m_flipbookMode) {
        opts.fps      = m_fps;
        opts.loop     = m_loopType;
        opts.clipName = m_clipName;
    }
    m_lastPackOpts = opts;

    if (!moth_packer::Pack(m_inputImages, opts)) {
        spdlog::error("TexturePacker: packing failed.");
        m_editorLayer.ShowError("TexturePacker: packing failed.");
        return;
    }

    // Read the descriptor to discover which atlas images were produced.
    // Flipbook mode writes <filename>.flipbook.json; atlas mode writes <filename>.json.
    auto const jsonPath = m_flipbookMode
                        ? tmpDir / "preview.flipbook.json"
                        : tmpDir / "preview.json";
    nlohmann::json descriptor;
    try {
        std::ifstream f(jsonPath);
        if (!f.is_open()) {
            auto const msg = fmt::format("TexturePacker: could not open descriptor '{}'", jsonPath.string());
            spdlog::error("{}", msg);
            m_editorLayer.ShowError(msg);
            return;
        }
        f >> descriptor;
    } catch (std::exception const& e) {
        auto const msg = fmt::format("TexturePacker: failed to parse descriptor: {}", e.what());
        spdlog::error("{}", msg);
        m_editorLayer.ShowError(msg);
        return;
    }

    auto& assetCtx = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();

    if (m_flipbookMode) {
        // Flipbook descriptor: { "image": "preview.png", "frames": [...], "clips": [...] }
        std::string const atlasFilename = descriptor.value("image", "");
        if (!atlasFilename.empty()) {
            AtlasPreview preview;
            preview.tempPath = tmpDir / atlasFilename;
            preview.image    = assetCtx.ImageFromFile(preview.tempPath);
            if (preview.image) {
                preview.width  = preview.image->GetWidth();
                preview.height = preview.image->GetHeight();
            }
            int frameIdx = 0;
            for (auto const& frameEntry : descriptor.value("frames", nlohmann::json::array())) {
                preview.imageNames.push_back("Frame " + std::to_string(frameIdx));
                // Flipbook frames store x/y/w/h as top-level keys (not nested under "rect")
                preview.imageRects.push_back({
                    frameEntry.value("x", 0), frameEntry.value("y", 0),
                    frameEntry.value("w", 0), frameEntry.value("h", 0) });
                ++frameIdx;
            }
            m_previewAtlases.push_back(std::move(preview));
        }
    } else {
        // Atlas descriptor: { "atlases": [{ "atlas": "...", "images": [...] }] }
        for (auto const& atlasEntry : descriptor.value("atlases", nlohmann::json::array())) {
            AtlasPreview preview;
            std::string const atlasFilename = atlasEntry.value("atlas", "");
            if (atlasFilename.empty()) {
                continue;
            }
            preview.tempPath = tmpDir / atlasFilename;
            preview.image    = assetCtx.ImageFromFile(preview.tempPath);
            if (preview.image) {
                preview.width  = preview.image->GetWidth();
                preview.height = preview.image->GetHeight();
            }

            for (auto const& imgEntry : atlasEntry.value("images", nlohmann::json::array())) {
                std::string const name = imgEntry.value("path", "");
                if (!name.empty()) {
                    preview.imageNames.push_back(
                        std::filesystem::path(name).filename().string());
                    auto const r = imgEntry.value("rect", nlohmann::json::object());
                    if (r.is_object()) {
                        preview.imageRects.push_back({
                            r.value("x", 0), r.value("y", 0),
                            r.value("w", 0), r.value("h", 0) });
                    } else {
                        preview.imageRects.push_back({ 0, 0, 0, 0 });
                    }
                }
            }

            m_previewAtlases.push_back(std::move(preview));
        }
    }

    m_hasPacked = true;
    spdlog::info("TexturePacker: packed into {} atlas{}",
        m_previewAtlases.size(),
        m_previewAtlases.size() != 1 ? "es" : "");
}

// ---- Save ---------------------------------------------------------------

void TexturePacker::DoSave() {
    if (!m_hasPacked) {
        return;
    }

    moth_packer::PackOptions opts = m_lastPackOpts;
    opts.outputPath    = m_saveDir;
    opts.filename      = m_saveFilename;
    opts.forceOverwrite = true;

    if (!moth_packer::Pack(m_inputImages, opts)) {
        spdlog::error("TexturePacker: save failed.");
        m_editorLayer.ShowError("TexturePacker: save failed.");
    } else {
        spdlog::info("TexturePacker: saved to '{}/{}'",
            m_saveDir, m_saveFilename);
    }
}

// ---- Output panel -------------------------------------------------------

void TexturePacker::DrawOutputPanel() {
    ImGui::SeparatorText("Output Atlases");

    if (!m_hasPacked) {
        ImGui::TextDisabled("Pack to see output.");
        return;
    }

    if (m_previewAtlases.empty()) {
        ImGui::TextDisabled("Pack succeeded but produced no atlases (all images exceeded max size).");
        return;
    }

    // ---- Atlas list ----
    float const listH = std::floor(ImGui::GetContentRegionAvail().y * 0.20f);
    if (ImGui::BeginChild("##output_list", ImVec2(0, listH),
            ImGuiChildFlags_Border, ImGuiWindowFlags_None)) {
        for (int i = 0; i < static_cast<int>(m_previewAtlases.size()); ++i) {
            auto const& atlas = m_previewAtlases[i];
            char label[64];
            snprintf(label, sizeof(label), "Atlas %d  (%d x %d, %d images)",
                i, atlas.width, atlas.height,
                static_cast<int>(atlas.imageNames.size()));
            ImGui::PushID(i);
            if (ImGui::Selectable(label, m_selectedOutput == i)) {
                if (m_selectedOutput != i) {
                    m_selectedOutput = i;
                    m_outputZoom = 0.0f;
                }
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();

    // ---- Zoom toolbar for atlas preview ----
    bool const hasOutputPreview = (m_selectedOutput >= 0 &&
                                   m_selectedOutput < static_cast<int>(m_previewAtlases.size()) &&
                                   m_previewAtlases[m_selectedOutput].image);
    if (hasOutputPreview) {
        if (m_outputZoom > 0.0f) {
            ImGui::Text("%.0f%%", m_outputZoom * 100.0f);
        } else {
            ImGui::TextDisabled("Fit");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit##out")) { m_outputZoom = 0.0f; }
        ImGui::SameLine();
        if (ImGui::SmallButton("+##out")) {
            auto const& atlas = m_previewAtlases[m_selectedOutput];
            float const srcW = static_cast<float>(atlas.width);
            float const srcH = static_cast<float>(atlas.height);
            float const avW  = ImGui::GetContentRegionAvail().x;
            float const avH  = ImGui::GetContentRegionAvail().y * 0.38f;
            float fitScale = 1.0f;
            if (srcW > 0.0f && srcH > 0.0f) {
                fitScale = std::min({ avW / srcW, avH / srcH, 1.0f });
            }
            float const cur = (m_outputZoom <= 0.0f) ? fitScale : m_outputZoom;
            m_outputZoom = std::min(cur * 1.25f, 16.0f);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("-##out")) {
            auto const& atlas = m_previewAtlases[m_selectedOutput];
            float const srcW = static_cast<float>(atlas.width);
            float const srcH = static_cast<float>(atlas.height);
            float const avW  = ImGui::GetContentRegionAvail().x;
            float const avH  = ImGui::GetContentRegionAvail().y * 0.38f;
            float fitScale = 1.0f;
            if (srcW > 0.0f && srcH > 0.0f) {
                fitScale = std::min({ avW / srcW, avH / srcH, 1.0f });
            }
            float const cur = (m_outputZoom <= 0.0f) ? fitScale : m_outputZoom;
            m_outputZoom = std::max(cur / 1.25f, 0.05f);
        }
    }

    // ---- Preview of selected atlas ----
    float const previewH = std::floor(ImGui::GetContentRegionAvail().y * 0.38f);
    if (ImGui::BeginChild("##output_preview", ImVec2(0, previewH),
            ImGuiChildFlags_Border, ImGuiWindowFlags_HorizontalScrollbar)) {
        if (hasOutputPreview) {
            auto const& atlas = m_previewAtlases[m_selectedOutput];
            ImGui::Text("Atlas %d: %d x %d px", m_selectedOutput, atlas.width, atlas.height);
            float const avW  = ImGui::GetContentRegionAvail().x;
            float const avH  = ImGui::GetContentRegionAvail().y;
            float const srcW = static_cast<float>(atlas.width);
            float const srcH = static_cast<float>(atlas.height);
            float fitScale = 1.0f;
            if (srcW > 0.0f && srcH > 0.0f) {
                fitScale = std::min({ avW / srcW, avH / srcH, 1.0f });
            }

            // Scroll wheel zoom
            if (ImGui::IsWindowHovered()) {
                float const wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0.0f) {
                    float const cur = (m_outputZoom <= 0.0f) ? fitScale : m_outputZoom;
                    m_outputZoom = std::clamp(cur * (wheel > 0.0f ? 1.1f : (1.0f / 1.1f)),
                                              0.05f, 16.0f);
                }
            }

            float const scale = (m_outputZoom <= 0.0f) ? fitScale : m_outputZoom;
            atlas.image->ImGui({
                static_cast<int>(srcW * scale),
                static_cast<int>(srcH * scale) });
        } else {
            ImGui::TextDisabled("Select an atlas to preview.");
        }
    }
    ImGui::EndChild();

    // ---- Packed image list for selected atlas ----
    if (m_selectedOutput >= 0 && m_selectedOutput < static_cast<int>(m_previewAtlases.size())) {
        auto const& atlas = m_previewAtlases[m_selectedOutput];
        float const imgsH = std::floor(ImGui::GetContentRegionAvail().y * 0.42f);
        if (ImGui::BeginChild("##output_imgs", ImVec2(0, imgsH),
                ImGuiChildFlags_Border, ImGuiWindowFlags_None)) {
            if (ImGui::BeginTable("##packed_imgs", 5,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                    ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit)) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("X",    ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Y",    ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("W",    ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("H",    ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableHeadersRow();

                for (int j = 0; j < static_cast<int>(atlas.imageNames.size()); ++j) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(atlas.imageNames[j].c_str());
                    if (j < static_cast<int>(atlas.imageRects.size())) {
                        auto const& rect = atlas.imageRects[j];
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%d", rect[0]);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%d", rect[1]);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%d", rect[2]);
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%d", rect[3]);
                    }
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    // ---- Save controls ----
    ImGui::SeparatorText("Save");

    auto const currentPath = std::filesystem::current_path().string();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Output Dir");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
    ImGui::InputText("##save_dir", m_saveDir, sizeof(m_saveDir) - 1,
        ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::Button("...##sd")) {
        nfdchar_t* outPath = nullptr;
        if (NFD_PickFolder(currentPath.c_str(), &outPath) == NFD_OKAY && outPath != nullptr) {
            strncpy(m_saveDir, outPath, sizeof(m_saveDir) - 1);
            m_saveDir[sizeof(m_saveDir) - 1] = '\0';
            NFD_Free(outPath);
        }
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Filename");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##save_fn", m_saveFilename, sizeof(m_saveFilename) - 1);

    bool const canSave = m_saveDir[0] != '\0' && m_saveFilename[0] != '\0';
    if (!canSave) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Save", ImVec2(-FLT_MIN, 0))) {
        DoSave();
    }
    if (!canSave) {
        ImGui::EndDisabled();
    }
}
