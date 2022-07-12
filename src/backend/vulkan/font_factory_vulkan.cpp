#include "common.h"
#include "font_factory_vulkan.h"

namespace backend::vulkan {
    FontFactory::FontFactory() {
    }

    std::unique_ptr<moth_ui::IFont> FontFactory::GetDefaultFont(int size) {
        return nullptr;
    }

    std::vector<std::string> FontFactory::GetFontNameList() {
        return {};
    }

    std::unique_ptr<moth_ui::IFont> FontFactory::GetFont(char const* name, int size) {
        return nullptr;
    }
}
