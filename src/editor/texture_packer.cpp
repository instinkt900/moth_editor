#include "common.h"
#include "texture_packer.h"
#include "app.h"
#include "image.h"

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

extern App* g_App;

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
                ImGui::Image(m_outputTexture.get(), ImVec2(static_cast<float>(m_textureWidth), static_cast<float>(m_textureHeight)));
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

void TexturePacker::CommitPack(int num, std::filesystem::path const& outputPath, int width, int height, std::vector<stbrp_rect>& rects, std::vector<ImageDetails> const& images) {
    auto renderer = g_App->GetRenderer();
    auto outputTexture = CreateTextureRef(SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height));
    auto const oldRenderTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, outputTexture.get());
    SDL_SetTextureBlendMode(outputTexture.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    nlohmann::json packDetails;
    for (auto&& rect : rects) {
        if (rect.was_packed) {
            auto const imagePath = images[rect.id].path;
            auto const image = moth_ui::Context::GetCurrentContext()->GetImageFactory().GetImage(images[rect.id].path);
            auto const internalImage = static_cast<Image*>(image.get());
            auto const texture = internalImage->GetTexture();

            SDL_Rect destRect;
            destRect.x = rect.x;
            destRect.y = rect.y;
            destRect.w = rect.w;
            destRect.h = rect.h;

            SDL_RenderCopy(renderer, texture.get(), nullptr, &destRect);

            nlohmann::json details;
            auto const relativePath = std::filesystem::relative(imagePath, outputPath);
            details["path"] = relativePath;
            details["rect"] = moth_ui::MakeRect(destRect.x, destRect.y, destRect.w, destRect.h);
            packDetails.push_back(details);
        }
    }

    // save packed image
    auto const imagePackName = fmt::format("packed_{}.png", num);
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
    SurfaceRef surface = CreateSurfaceRef(SDL_CreateRGBSurface(0, width, height, 32, rmask, gmask, bmask, amask));
    SDL_RenderReadPixels(renderer, nullptr, surface->format->format, surface->pixels, surface->pitch);
    IMG_SavePNG(surface.get(), (outputPath / imagePackName).string().c_str());

    SDL_SetRenderTarget(renderer, oldRenderTarget);

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
        if (!images.empty())
        {
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
            while (!stbRects.empty()) {

                int currentWidth = minWidth;
                int currentHeight = minHeight;

                bool packed = false;
                while (!packed) {
                    while (!packed) {
                        stbrp_context stbContext;
                        stbrp_init_target(&stbContext, currentWidth, currentHeight, stbNodes.data(), static_cast<int>(stbNodes.size()));
                        auto const allPacked = stbrp_pack_rects(&stbContext, stbRects.data(), static_cast<int>(stbRects.size()));

                        if (allPacked || currentWidth >= maxWidth && currentHeight >= maxHeight) {
                            packed = true;
                            CommitPack(numPacks, outputPath, currentWidth, currentHeight, stbRects, images);
                        } else {
                            if (currentWidth >= maxWidth) {
                                break;
                            } else {
                                currentWidth = std::min(maxWidth, currentWidth * 2);
                            }
                        }
                    }

                    if (!packed) {
                        currentHeight = std::min(maxHeight, currentHeight * 2);
                    }
                }
            }
        }
    }
}
