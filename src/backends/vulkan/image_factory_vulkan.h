#pragma once

#include "moth_ui/iimage_factory.h"
#include "moth_ui/iimage.h"

class ImageFactoryVulkan : public moth_ui::IImageFactory {
public:
    ImageFactoryVulkan();
    virtual ~ImageFactoryVulkan() = default;

    void FlushCache() override;
    bool LoadTexturePack(std::filesystem::path const& path) override;

    std::unique_ptr<moth_ui::IImage> GetImage(std::filesystem::path const& path) override;
};
