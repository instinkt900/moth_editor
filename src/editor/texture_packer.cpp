#include "common.h"
#include "texture_packer.h"
#include "iapp.h"

#include "moth_ui/context.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/layout/layout_entity_image.h"

#include "imgui-filebrowser/imfilebrowser.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#undef min
#undef max

namespace {
    ImGui::FileBrowser s_fileBrowser;
    char* s_destPathBuffer = nullptr;
    char s_layoutPathBuffer[1024];
    char s_outputPathBuffer[1024];
    int s_minWidth = 256;
    int s_minHeight = 256;
    int s_maxWidth = 1024;
    int s_maxHeight = 1024;
}

extern IApp* g_App;

TexturePacker::TexturePacker() {
}

TexturePacker::~TexturePacker() {
}

void TexturePacker::Draw() {
    s_fileBrowser.Display();
    if (s_fileBrowser.HasSelected()) {
        strncpy(s_destPathBuffer, s_fileBrowser.GetSelected().string().c_str(), 1024);
    }

    if (m_open) {
        if (ImGui::Begin("Texture Packing", &m_open)) {
            ImGui::InputText("##layouts_path", s_layoutPathBuffer, 1024);
            ImGui::SameLine();
            if (ImGui::Button("...##layout")) {
                s_fileBrowser = ImGui::FileBrowser(ImGuiFileBrowserFlags_SelectDirectory);
                s_fileBrowser.SetTitle("Open..");
                s_fileBrowser.SetPwd();
                s_destPathBuffer = s_layoutPathBuffer;
                s_fileBrowser.Open();
            }
            ImGui::SameLine();
            ImGui::Text("Layouts path");

            ImGui::InputText("##output_path", s_outputPathBuffer, 1024);
            ImGui::SameLine();
            if (ImGui::Button("...##output")) {
                s_fileBrowser = ImGui::FileBrowser(ImGuiFileBrowserFlags_SelectDirectory);
                s_fileBrowser.SetTitle("Open..");
                s_fileBrowser.SetPwd();
                s_destPathBuffer = s_outputPathBuffer;
                s_fileBrowser.Open();
            }
            ImGui::SameLine();
            ImGui::Text("Output path");

            ImGui::InputInt("Min Width", &s_minWidth);
            ImGui::InputInt("Min Height", &s_minHeight);

            ImGui::InputInt("Max Width", &s_maxWidth);
            ImGui::InputInt("Max Height", &s_maxHeight);

            if (ImGui::Button("Pack")) {
                //Pack(s_pathBuffer, s_maxWidth, s_maxHeight);
                Pack(s_layoutPathBuffer, s_outputPathBuffer, s_minWidth, s_minHeight, s_maxWidth, s_maxHeight);
            }

            if (m_outputTexture) {
                imgui_ext::Image(m_outputTexture, m_textureWidth, m_textureHeight);
            }
        }
        ImGui::End();
    }
}

// recursively loads all layouts in a given path
void TexturePacker::CollectLayouts(std::filesystem::path const& path, std::vector<std::shared_ptr<moth_ui::Layout>>& layouts) {
    for (auto&& entry : std::filesystem::directory_iterator(path)) {
        if (std::filesystem::is_directory(entry.path())) {
            CollectLayouts(entry.path(), layouts);
        } else if (entry.path().has_extension() && entry.path().extension() == moth_ui::Layout::Extension) {
            std::shared_ptr<moth_ui::Layout> layout;
            auto const result = moth_ui::Layout::Load(entry.path(), &layout);
            if (result == moth_ui::Layout::LoadResult::Success) {
                layouts.push_back(layout);
            }
        }
    }
}

void TexturePacker::CollectImages(moth_ui::Layout const& layout, std::vector<ImageDetails>& images) {
    for (auto&& childEntity : layout.m_children) {
        if (childEntity->GetType() == moth_ui::LayoutEntityType::Image) {
            auto& imageEntity = static_cast<moth_ui::LayoutEntityImage&>(*childEntity);
            auto imagePath = layout.GetLoadedPath() / imageEntity.m_imagePath;
            // dont add duplicates
            if (std::end(images) == ranges::find_if(images, [&](auto const& detail) { return detail.path == imagePath; })) {
                auto image = moth_ui::Context::GetCurrentContext()->GetImageFactory().GetImage(imagePath);
                ImageDetails details;
                details.path = imagePath;
                details.dimensions = image->GetDimensions();
                images.push_back(details);
            }
        }
    }
}

int NextPowerOf2(int value) {
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    return value;
}

moth_ui::IntVec2 TexturePacker::FindOptimalDimensions(std::vector<stbrp_node>& nodes, std::vector<stbrp_rect>& rects, moth_ui::IntVec2 const& minPack, moth_ui::IntVec2 const& maxPack) {
    // collect some info about all the rects
    int minWidth = std::numeric_limits<int>::max();
    int minHeight = std::numeric_limits<int>::max();
    int maxWidth = std::numeric_limits<int>::min();
    int maxHeight = std::numeric_limits<int>::min();
    int totalArea = 0;
    int totalWidth = 0;
    int totalHeight = 0;

    for (auto&& rect : rects) {
        minWidth = std::min(minWidth, rect.w);
        minHeight = std::min(minHeight, rect.h);
        maxWidth = std::max(maxWidth, rect.w);
        maxHeight = std::max(maxHeight, rect.h);
        totalWidth += rect.w;
        totalHeight += rect.h;
        totalArea += rect.w * rect.h;
    }

    struct PackTest {
        moth_ui::IntVec2 m_dimensions;
        float m_ratio;
    };

    int const minDimX = NextPowerOf2(minPack.x);
    int const minDimY = NextPowerOf2(minPack.y);
    int const maxDimX = NextPowerOf2(maxPack.x);
    int const maxDimY = NextPowerOf2(maxPack.y);

    std::vector<PackTest> testDimensions;
    int curWidth = minDimX;
    while (curWidth <= maxDimX) {
        int curHeight = minDimY;
        while (curHeight <= maxDimY) {
            int const curArea = curWidth * curHeight;
            if (curArea > totalArea) {
                PackTest info;
                info.m_dimensions = moth_ui::IntVec2{ curWidth, curHeight };
                info.m_ratio = 0;
                testDimensions.push_back(info);
            }
            curHeight *= 2;
        }
        curWidth *= 2;
    }

    for (auto&& testDim : testDimensions) {
        stbrp_context stbContext;
        stbrp_init_target(&stbContext, testDim.m_dimensions.x, testDim.m_dimensions.y, nodes.data(), static_cast<int>(nodes.size()));
        auto const allPacked = stbrp_pack_rects(&stbContext, rects.data(), static_cast<int>(rects.size()));
        if (allPacked) {
            float testArea = static_cast<float>(testDim.m_dimensions.x * testDim.m_dimensions.y);
            testDim.m_ratio = totalArea / testArea;
        }
    }

    std::sort(std::begin(testDimensions), std::end(testDimensions), [](auto const& a, auto const& b) { return b.m_ratio < a.m_ratio; });
    return std::begin(testDimensions)->m_dimensions;
}

void TexturePacker::CommitPack(int num, std::filesystem::path const& outputPath, int width, int height, std::vector<stbrp_rect>& rects, std::vector<ImageDetails> const& images) {
    auto& graphics = g_App->GetGraphics();
    std::shared_ptr<moth_ui::IImage> outputTexture = graphics.CreateTarget(width, height);
    auto oldRenderTarget = graphics.GetTarget();
    graphics.SetTarget(outputTexture);
    graphics.SetBlendMode(outputTexture, backend::EBlendMode::Blend);
    graphics.SetColor(moth_ui::BasicColors::Black);
    graphics.Clear();

    nlohmann::json packDetails;
    for (auto&& rect : rects) {
        if (rect.was_packed) {
            auto const imagePath = images[rect.id].path;
            std::shared_ptr<moth_ui::IImage> image = moth_ui::Context::GetCurrentContext()->GetImageFactory().GetImage(images[rect.id].path);

            moth_ui::IntRect destRect = moth_ui::MakeRect(rect.x, rect.y, rect.w, rect.h);

            graphics.SetBlendMode(image, backend::EBlendMode::None);
            graphics.SetColorMod(image, moth_ui::BasicColors::White);
            graphics.DrawImage(image, nullptr, &destRect);

            nlohmann::json details;
            auto const relativePath = std::filesystem::relative(imagePath, outputPath);
            details["path"] = relativePath.string();
            details["rect"] = destRect;
            packDetails.push_back(details);
        }
    }

    // save packed image
    auto const imagePackName = fmt::format("packed_{}.png", num);
    graphics.DrawToPNG(outputPath / imagePackName);

    graphics.SetTarget(oldRenderTarget);    

    // save description
    auto const packDetailsName = fmt::format("packed_{}.json", num);
    std::ofstream ofile(outputPath / packDetailsName);
    if (ofile.is_open()) {
        nlohmann::json detailsRoot;
        detailsRoot["images"] = packDetails;
        ofile << detailsRoot;
    }

    // remove packed rects
    rects.erase(ranges::remove_if(rects, [](auto const& r) { return r.was_packed; }), std::end(rects));

    m_outputTexture = outputTexture;
    m_textureWidth = width;
    m_textureHeight = height;
}

void TexturePacker::Pack(std::filesystem::path const& inputPath, std::filesystem::path const& outputPath, int minWidth, int minHeight, int maxWidth, int maxHeight) {
    if (std::filesystem::exists(inputPath)) {
        // recursively collect all layouts in the given path
        std::vector<std::shared_ptr<moth_ui::Layout>> layouts;
        CollectLayouts(inputPath, layouts);

        // collect a list of image references within all layouts
        std::vector<ImageDetails> images;
        for (auto&& layout : layouts) {
            CollectImages(*layout, images);
        }

        // pack images into a number of packs
        if (!images.empty()) {
            // add all image rects to a single bucket
            std::vector<stbrp_rect> stbRects;
            stbRects.reserve(images.size());
            int i = 0;
            for (auto&& image : images) {
                stbrp_rect r;
                r.id = i;
                r.w = image.dimensions.x;
                r.h = image.dimensions.y;
                // dont add images larger than max pack size
                if (r.w <= maxWidth && r.h <= maxHeight) {
                    stbRects.push_back(r);
                }
                ++i;
            }

            std::vector<stbrp_node> stbNodes(maxWidth * 2);

            // keep creating packs until we run out of images.
            int numPacks = 0;
            auto const& packDim = FindOptimalDimensions(stbNodes, stbRects, { minWidth, minHeight }, { maxWidth, maxHeight });

            stbrp_context stbContext;
            stbrp_init_target(&stbContext, packDim.x, packDim.y, stbNodes.data(), static_cast<int>(stbNodes.size()));
            stbrp_pack_rects(&stbContext, stbRects.data(), static_cast<int>(stbRects.size()));
            CommitPack(numPacks, outputPath, packDim.x, packDim.y, stbRects, images);
        }
    }
}
