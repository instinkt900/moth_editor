#include "common.h"
#include "font_factory_vulkan.h"

FontFactoryVulkan::FontFactoryVulkan() {
}

std::unique_ptr<moth_ui::IFont> FontFactoryVulkan::GetDefaultFont(int size) {
    return nullptr;
}

std::vector<std::string> FontFactoryVulkan::GetFontNameList() {
    return {};
}

std::unique_ptr<moth_ui::IFont> FontFactoryVulkan::GetFont(char const* name, int size) {
    return nullptr;
}
