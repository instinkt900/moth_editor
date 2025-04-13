#pragma once

#include "moth_ui/context.h"
#include "stb_rect_pack.h"

class TexturePacker {
public:
    TexturePacker(moth_ui::Context& context);
    ~TexturePacker();

    void Open() {
        m_open = true;
    }
    void Draw();

private:
    moth_ui::Context& m_context;
    bool m_open = false;
    std::shared_ptr<moth_ui::ITarget> m_outputTexture;
    int m_textureWidth = 0;
    int m_textureHeight = 0;

    struct ImageDetails {
        std::filesystem::path path;
        moth_ui::IntVec2 dimensions;
    };

    void Pack(std::filesystem::path const& inputPath, std::filesystem::path const& outputPath, int minWidth, int minHeight, int maxWidth, int maxHeight);
    void CommitPack(int num, std::filesystem::path const& outputPath, int width, int height, std::vector<stbrp_rect>& rects, std::vector<ImageDetails> const& images);
    void CollectLayouts(std::filesystem::path const& path, std::vector<std::shared_ptr<moth_ui::Layout>>& layouts);
    void CollectImages(moth_ui::Layout const& layout, std::vector<ImageDetails>& images);
    moth_ui::IntVec2 FindOptimalDimensions(std::vector<stbrp_node>& nodes, std::vector<stbrp_rect>& rects, moth_ui::IntVec2 const& minPack, moth_ui::IntVec2 const& maxPack);
};
