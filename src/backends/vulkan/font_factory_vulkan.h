#pragma once

#include "moth_ui/ifont_factory.h"
#include "moth_ui/ifont.h"

class FontFactoryVulkan : public moth_ui::IFontFactory {
public:
    FontFactoryVulkan();
    virtual ~FontFactoryVulkan() = default;

    std::unique_ptr<moth_ui::IFont> GetDefaultFont(int size) override;
    std::vector<std::string> GetFontNameList() override;
    std::unique_ptr<moth_ui::IFont> GetFont(char const* name, int size) override;
};
