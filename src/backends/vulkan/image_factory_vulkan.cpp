#include "common.h"
#include "image_factory_vulkan.h"

ImageFactoryVulkan::ImageFactoryVulkan() {
}

void ImageFactoryVulkan::FlushCache() {
}

bool ImageFactoryVulkan::LoadTexturePack(std::filesystem::path const& path) {
    return true;
}

std::unique_ptr<moth_ui::IImage> ImageFactoryVulkan::GetImage(std::filesystem::path const& path) {
    return nullptr;
}
