#pragma once

#include "moth_ui/ifont.h"
#include "vulkan_buffer.h"
#include "vulkan_graphics.h"

namespace backend::vulkan {
    class Font : public moth_ui::IFont {
    public:
        static std::shared_ptr<Font> Load(char const* path, int size, Context& context, Graphics& graphics);
        virtual ~Font();

        int GetGlyphIndex(int charCode) const;
        moth_ui::IntVec2 GetGlyphSize(int charCode) const;

        VkDescriptorSet GetVKDescriptorSet() const { return m_vkDescriptorSet; }

        int32_t GetLineHeight() const { return m_lineHeight; }
        int32_t GetStringWidth(std::string_view const& str) const;
        int32_t GetColumnHeight(std::string const& str, int32_t width) const;

    private:
        Font(FT_Face face, int size, Context& context, Graphics& graphics);

        struct GlyphInfo {
            moth_ui::IntVec2 Size;
            moth_ui::IntVec2 Advance;
            moth_ui::FloatVec2 UV0;
            moth_ui::FloatVec2 UV1;
        };

        int32_t m_lineHeight;
        std::map<int, int> m_charCodeToIndex;
        std::vector<GlyphInfo> m_glyphInfo;
        std::unique_ptr<Image> m_glyphAtlas;

        VkDescriptorSet m_vkDescriptorSet;
        std::unique_ptr<Buffer> m_storageBuffer;
    };
}
