#include "common.h"
#include "editor_panel_asset_list.h"
#include "../editor_layer.h"
#include "moth_ui/layout/layout.h"
#include "moth_graphics/graphics/iimage.h"
#include "moth_graphics/graphics/igraphics.h"
#include "moth_graphics/graphics/surface_context.h"
#include "moth_graphics/graphics/asset_context.h"
#include "moth_graphics/graphics/image_factory.h"

static std::string DragDropString;

namespace {
    std::vector<std::string> const s_imageExtensions{
        ".jpg",
        ".jpeg",
        ".png"
    };

    bool IsImageExtension(std::string const& ext) {
        return std::end(s_imageExtensions) != ranges::find(s_imageExtensions, ext);
    }

    bool IsFlipbook(std::filesystem::path const& path) {
        return path.extension() == ".json" && path.stem().extension() == ".flipbook";
    }

    bool IsSupportedFile(std::filesystem::path const& path) {
        if (path.extension().string() == moth_ui::Layout::FullExtension) {
            return true;
        }
        if (IsFlipbook(path)) {
            return true;
        }
        if (!path.has_extension()) {
            return false;
        }
        return IsImageExtension(path.extension().string());
    }
}

EditorPanelAssetList::EditorPanelAssetList(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Asset List", visible, true)
    , m_contentList(".") {

    m_contentList.SetDisplayNameAction([](std::filesystem::path const& path) {
        return path.filename().string();
    });

    m_contentList.SetDoubleClickAction([this](std::filesystem::path const& path) {
        if (path.extension().string() == moth_ui::Layout::FullExtension) {
            m_editorLayer.LoadLayout(path);
        }
    });

    m_contentList.SetPerEntryAction([this](std::filesystem::path const& path) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            DragDropString = path.string();
            if (path.extension().string() == moth_ui::Layout::FullExtension) {
                ImGui::SetDragDropPayload("layout_path", &DragDropString, sizeof(std::string));
                ImGui::Text("%s", DragDropString.c_str());
            } else if (IsFlipbook(path)) {
                ImGui::SetDragDropPayload("flipbook_path", &DragDropString, sizeof(std::string));
                ImGui::Text("%s", DragDropString.c_str());
            } else {
                ImGui::SetDragDropPayload("image_path", &DragDropString, sizeof(std::string));
                ImGui::Text("%s", DragDropString.c_str());
            }
            ImGui::EndDragDropSource();
        }

        if (IsImageExtension(path.extension().string()) && ImGui::IsItemHovered()) {
            auto const key = path.string();
            if (m_imageCache.count(key) == 0) {
                auto& factory = m_editorLayer.GetGraphics().GetSurfaceContext().GetAssetContext().GetImageFactory();
                m_imageCache[key] = factory.GetImage(path);
            }
            auto const* image = m_imageCache.at(key).get();
            if (image != nullptr) {
                static constexpr int MaxTooltipDim = 256;
                float const scale = std::min(
                    static_cast<float>(MaxTooltipDim) / static_cast<float>(image->GetWidth()),
                    static_cast<float>(MaxTooltipDim) / static_cast<float>(image->GetHeight()));
                int const w = std::max(1, static_cast<int>(static_cast<float>(image->GetWidth()) * scale));
                int const h = std::max(1, static_cast<int>(static_cast<float>(image->GetHeight()) * scale));
                ImGui::BeginTooltip();
                imgui_ext::Image(image, w, h);
                ImGui::EndTooltip();
            }
        }
    });

    m_contentList.SetDisplayFilter([](std::filesystem::path const& path) {
        if (std::filesystem::is_directory(path)) {
            return true;
        }
        return IsSupportedFile(path);
    });
}

EditorPanelAssetList::~EditorPanelAssetList() = default;

void EditorPanelAssetList::Refresh() {
    m_contentList.Refresh();
    m_imageCache.clear();
    try {
        m_lastDirWriteTime = std::filesystem::last_write_time(m_contentList.GetPath());
    } catch (std::exception const&) {
        m_lastDirWriteTime = {};
    }
}

void EditorPanelAssetList::DrawContents() {
    try {
        auto const writeTime = std::filesystem::last_write_time(m_contentList.GetPath());
        if (writeTime != m_lastDirWriteTime) {
            Refresh();
        }
    } catch (std::exception const&) {
    }
    auto const currentPath = m_contentList.GetPath();

    std::vector<std::string> parts;
    for (auto const& part : currentPath) {
        auto const s = part.string();
        if (!s.empty() && part != currentPath.root_path()) {
            parts.push_back(s);
        }
    }

    static constexpr int MaxComponents = 2;
    std::string displayPath;
    if (static_cast<int>(parts.size()) <= MaxComponents) {
        displayPath = currentPath.string();
    } else {
        std::filesystem::path truncated;
        for (int i = static_cast<int>(parts.size()) - MaxComponents; i < static_cast<int>(parts.size()); ++i) {
            truncated /= parts[i];
        }
        displayPath = ".../" + truncated.string();
    }

    ImGui::TextUnformatted(displayPath.c_str());
    if (ImGui::IsItemHovered()) {
        auto const fullPathStr = currentPath.string();
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(300.0f);
        ImGui::TextUnformatted(fullPathStr.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    ImGui::Separator();
    m_contentList.Draw();
}
