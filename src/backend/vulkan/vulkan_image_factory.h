#pragma once

#include "moth_ui/iimage_factory.h"
#include "moth_ui/iimage.h"
#include "vulkan_context.h"
#include "vulkan_image.h"
#include "vulkan_graphics.h"

namespace backend::vulkan {
    class ImageFactory : public moth_ui::IImageFactory {
    public:
        ImageFactory(Context& m_context, Graphics& graphics);
        virtual ~ImageFactory() = default;

        void FlushCache() override;
        bool LoadTexturePack(std::filesystem::path const& path) override;

        std::unique_ptr<moth_ui::IImage> GetImage(std::filesystem::path const& path) override;

    private:
        Context& m_context;

        struct ImageDesc {
            std::shared_ptr<Image> m_texture;
            moth_ui::IntRect m_sourceRect;
            std::string m_path;
        };

        std::unordered_map<std::string, ImageDesc> m_cachedImages;
    };
}
