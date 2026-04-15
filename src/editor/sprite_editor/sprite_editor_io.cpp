#include "common.h"
#include "sprite_editor.h"
#include "editor/editor_layer.h"

#include "moth_graphics/graphics/igraphics.h"
#include "moth_graphics/graphics/surface_context.h"
#include "moth_graphics/graphics/asset_context.h"
#include "moth_graphics/graphics/spritesheet_factory.h"

void SpriteEditor::LoadSpriteSheet(std::filesystem::path const& path) {
    // Load and validate before touching any editor state so a failed load
    // leaves the current document intact.
    auto& assetContext = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext();
    auto newSheet = assetContext.GetSpriteSheetFactory().GetSpriteSheet(path);
    if (!newSheet) {
        spdlog::error("Failed to load sprite sheet: {}", path.string());
        return;
    }

    ClearSpriteActions();
    m_selectedFrame = -1;
    m_selectedClip = -1;
    m_clipPlaying = false;
    m_clipCurrentStep = 0;
    m_clipElapsedMs = 0.0f;
    m_zoom = -1.0f; // trigger auto-fit on next draw
    m_frames.clear();
    m_imagePathBuffer[0] = '\0';
    m_spriteSheet = std::move(newSheet);

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

    ClearSpriteActions();

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
        json["image"] = relImage.empty() ? imagePath.filename().string() : relImage.string();
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
        default:                                                     loopStr = "stop";  break;
        }
        clipObj["loop"] = loopStr;

        nlohmann::json stepsJson = nlohmann::json::array();
        int const frameCount = static_cast<int>(m_frames.size());
        for (auto const& step : entry.desc.frames) {
            nlohmann::json stepObj;
            int const safeFrameIdx = (frameCount > 0)
                ? std::clamp(step.frameIndex, 0, frameCount - 1)
                : 0;
            if (safeFrameIdx != step.frameIndex) {
                spdlog::warn("SpriteEditor: clip '{}' step has out-of-range frameIndex {}, saving as {}",
                    entry.name, step.frameIndex, safeFrameIdx);
            }
            stepObj["frame"]       = safeFrameIdx;
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
