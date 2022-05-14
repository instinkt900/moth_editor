#pragma once

#include "moth_ui/utils/color.h"
#include "moth_ui/utils/vector_serialization.h"

struct EditorConfig {
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
}

inline void from_json(nlohmann::json j, EditorConfig& config) {
    config.CanvasBackgroundColor = j.value("CanvasBackgroundColor", config.CanvasBackgroundColor);
    config.CanvasOutlineColor = j.value("CanvasOutlineColor", config.CanvasOutlineColor);
    config.CanvasColor = j.value("CanvasColor", config.CanvasColor);
    config.CanvasGridColorMajor = j.value("CanvasGridColorMajor", config.CanvasGridColorMajor);
    config.CanvasGridColorMinor = j.value("CanvasGridColorMinor", config.CanvasGridColorMinor);
    config.CanvasSize = j.value("CanvasSize", config.CanvasSize);
    config.CanvasGridSpacing = j.value("CanvasGridSpacing", config.CanvasGridSpacing);
    config.CanvasGridMajorFactor = j.value("CanvasGridMajorFactor", config.CanvasGridMajorFactor);
    config.SelectionColor = j.value("SelectionColor", config.SelectionColor);
    config.SelectionSliceColor = j.value("SelectionSliceColor", config.SelectionSliceColor);
    config.PreviewSourceRectColor = j.value("PreviewSourceRectColor", config.PreviewSourceRectColor);
    config.PreviewImageSliceColor = j.value("PreviewImageSliceColor", config.PreviewImageSliceColor);
}
