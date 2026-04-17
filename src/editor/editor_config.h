#pragma once

#include "moth_ui/utils/color.h"
#include "moth_ui/utils/vector_serialization.h"

#include <string>
#include <vector>

struct EditorConfig {
    struct Resolution {
        std::string name;
        int width = 0;
        int height = 0;
    };

    moth_ui::Color CanvasBackgroundColor = moth_ui::Color{ 0.67f, 0.67f, 0.67f, 1.0f };
    moth_ui::Color CanvasOutlineColor = moth_ui::Color{ 0.0f, 0.0f, 0.0f, 1.0f };
    moth_ui::Color CanvasColor = moth_ui::Color{ 1.0f, 1.0f, 1.0f, 1.0f };
    moth_ui::Color CanvasGridColorMinor = moth_ui::Color{ 0.0f, 0.0f, 0.0f, 0.08f };
    moth_ui::Color CanvasGridColorMajor = moth_ui::Color{ 0.0f, 0.0f, 0.0f, 0.23f };
    moth_ui::Color SelectionColor = moth_ui::Color{ 0.0f, 0.0f, 1.0f, 1.0f };
    moth_ui::Color SelectionSliceColor = moth_ui::Color{ 0.0f, 0.27f, 0.47f, 1.0f };
    moth_ui::Color PreviewSourceRectColor = moth_ui::Color{ 1.0f, 1.0f, 0.0f, 1.0f };
    moth_ui::Color PreviewImageSliceColor = moth_ui::Color{ 0.0f, 0.27f, 0.47f, 1.0f };

    moth_ui::IntVec2 CanvasSize{ 640, 480 };
    int CanvasGridSpacing = 10;
    int CanvasGridMajorFactor = 8;

    int MinAnimationFrame = 0;
    int MaxAnimationFrame = 100;
    int TotalAnimationFrames = 300;
    int CurrentAnimationFrame = 0;

    bool SnapToGrid = false;
    bool SnapToAngle = false;
    float SnapAngle = 15.0f;

    bool AutoSaveEnabled = false;
    int AutoSaveIntervalMinutes = 5;
    int AutoSaveMaxVersions = 5;

    moth_ui::Color SpriteEditorNormalColor = moth_ui::Color{ 1.0f, 1.0f, 0.0f, 200.0f / 255.0f };
    moth_ui::Color SpriteEditorSelectedColor = moth_ui::Color{ 0.0f, 1.0f, 1.0f, 1.0f };
    int SpriteEditorRectThickness = 1;

    // clang-format off
    std::vector<Resolution> Resolutions = {
        { "320 × 180  (16:9)",   320,  180 },
        { "320 × 240  (4:3)",    320,  240 },
        { "480 × 270  (16:9)",   480,  270 },
        { "640 × 360  (16:9)",   640,  360 },
        { "640 × 480  (4:3)",    640,  480 },
        { "800 × 600  (4:3)",    800,  600 },
        { "960 × 540  (16:9)",   960,  540 },
        { "1024 × 576 (16:9)",  1024,  576 },
        { "1024 × 768 (4:3)",   1024,  768 },
        { "1280 × 720 (16:9)",  1280,  720 },
        { "1280 × 800 (16:10)", 1280,  800 },
        { "1280 × 1024 (5:4)",  1280, 1024 },
        { "1366 × 768 (16:9)",  1366,  768 },
        { "1600 × 900 (16:9)",  1600,  900 },
        { "1920 × 1080 (16:9)", 1920, 1080 },
        { "1920 × 1200 (16:10)",1920, 1200 },
        { "2560 × 1440 (16:9)", 2560, 1440 },
        { "3840 × 2160 (16:9)", 3840, 2160 },
        { "360 × 640  (9:16)",   360,  640 },
        { "375 × 667  (9:16)",   375,  667 },
        { "414 × 896  (9:16)",   414,  896 },
        { "768 × 1024 (3:4)",    768, 1024 },
    };
    // clang-format on
};

inline void to_json(nlohmann::json& j, EditorConfig const& config) {
    j["CanvasBackgroundColor"] = config.CanvasBackgroundColor;
    j["CanvasOutlineColor"] = config.CanvasOutlineColor;
    j["CanvasColor"] = config.CanvasColor;
    j["CanvasGridColorMajor"] = config.CanvasGridColorMajor;
    j["CanvasGridColorMinor"] = config.CanvasGridColorMinor;
    j["CanvasSize"] = config.CanvasSize;
    j["CanvasGridSpacing"] = config.CanvasGridSpacing;
    j["CanvasGridMajorFactor"] = config.CanvasGridMajorFactor;
    j["SelectionColor"] = config.SelectionColor;
    j["SelectionSliceColor"] = config.SelectionSliceColor;
    j["PreviewSourceRectColor"] = config.PreviewSourceRectColor;
    j["PreviewImageSliceColor"] = config.PreviewImageSliceColor;
    j["MinAnimationFrame"] = config.MinAnimationFrame;
    j["MaxAnimationFrame"] = config.MaxAnimationFrame;
    j["TotalAnimationFrames"] = config.TotalAnimationFrames;
    j["CurrentAnimationFrame"] = config.CurrentAnimationFrame;
    j["SnapToGrid"] = config.SnapToGrid;
    j["SnapToAngle"] = config.SnapToAngle;
    j["SnapAngle"] = config.SnapAngle;
    j["AutoSaveEnabled"] = config.AutoSaveEnabled;
    j["AutoSaveIntervalMinutes"] = config.AutoSaveIntervalMinutes;
    j["AutoSaveMaxVersions"] = config.AutoSaveMaxVersions;
    j["SpriteEditorNormalColor"] = config.SpriteEditorNormalColor;
    j["SpriteEditorSelectedColor"] = config.SpriteEditorSelectedColor;
    j["SpriteEditorRectThickness"] = config.SpriteEditorRectThickness;

    auto resArray = nlohmann::json::array();
    for (auto const& r : config.Resolutions) {
        resArray.push_back({ { "name", r.name }, { "width", r.width }, { "height", r.height } });
    }
    j["Resolutions"] = resArray;
}

inline void from_json(nlohmann::json j, EditorConfig& config) {
    config.CanvasBackgroundColor = j.value("CanvasBackgroundColor", config.CanvasBackgroundColor);
    config.CanvasOutlineColor = j.value("CanvasOutlineColor", config.CanvasOutlineColor);
    config.CanvasColor = j.value("CanvasColor", config.CanvasColor);
    config.CanvasGridColorMajor = j.value("CanvasGridColorMajor", config.CanvasGridColorMajor);
    config.CanvasGridColorMinor = j.value("CanvasGridColorMinor", config.CanvasGridColorMinor);
    auto const rawSize = j.value("CanvasSize", config.CanvasSize);
    config.CanvasSize = { std::max(1, rawSize.x), std::max(1, rawSize.y) };
    config.CanvasGridSpacing = std::max(j.value("CanvasGridSpacing", config.CanvasGridSpacing), 1);
    config.CanvasGridMajorFactor = std::max(j.value("CanvasGridMajorFactor", config.CanvasGridMajorFactor), 1);
    config.SelectionColor = j.value("SelectionColor", config.SelectionColor);
    config.SelectionSliceColor = j.value("SelectionSliceColor", config.SelectionSliceColor);
    config.PreviewSourceRectColor = j.value("PreviewSourceRectColor", config.PreviewSourceRectColor);
    config.PreviewImageSliceColor = j.value("PreviewImageSliceColor", config.PreviewImageSliceColor);
    config.MinAnimationFrame = j.value("MinAnimationFrame", config.MinAnimationFrame);
    config.MaxAnimationFrame = j.value("MaxAnimationFrame", config.MaxAnimationFrame);
    config.TotalAnimationFrames = j.value("TotalAnimationFrames", config.TotalAnimationFrames);
    config.CurrentAnimationFrame = j.value("CurrentAnimationFrame", config.CurrentAnimationFrame);
    config.SnapToGrid = j.value("SnapToGrid", config.SnapToGrid);
    config.SnapToAngle = j.value("SnapToAngle", config.SnapToAngle);
    config.SnapAngle = j.value("SnapAngle", config.SnapAngle);
    config.AutoSaveEnabled = j.value("AutoSaveEnabled", config.AutoSaveEnabled);
    config.AutoSaveIntervalMinutes = std::clamp(j.value("AutoSaveIntervalMinutes", config.AutoSaveIntervalMinutes), 1, 1440);
    config.AutoSaveMaxVersions = std::clamp(j.value("AutoSaveMaxVersions", config.AutoSaveMaxVersions), 1, 100);
    config.SpriteEditorNormalColor = j.value("SpriteEditorNormalColor", config.SpriteEditorNormalColor);
    config.SpriteEditorSelectedColor = j.value("SpriteEditorSelectedColor", config.SpriteEditorSelectedColor);
    config.SpriteEditorRectThickness = std::max(j.value("SpriteEditorRectThickness", config.SpriteEditorRectThickness), 1);

    if (j.contains("Resolutions") && j["Resolutions"].is_array()) {
        config.Resolutions.clear();
        for (auto const& entry : j["Resolutions"]) {
            EditorConfig::Resolution r;
            r.name = entry.value("name", std::string{});
            r.width = entry.value("width", 0);
            r.height = entry.value("height", 0);
            if (r.width > 0 && r.height > 0 && !r.name.empty()) {
                config.Resolutions.push_back(std::move(r));
            }
        }
    }
}
