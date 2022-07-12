#include "common.h"
#include "image_factory_vulkan.h"

namespace backend::vulkan {
    ImageFactory::ImageFactory() {
    }

    void ImageFactory::FlushCache() {
    }

    bool ImageFactory::LoadTexturePack(std::filesystem::path const& path) {
        return true;
    }

    std::unique_ptr<moth_ui::IImage> ImageFactory::GetImage(std::filesystem::path const& path) {
        return nullptr;
    }
}
