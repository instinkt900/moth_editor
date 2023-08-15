#pragma once

#include "moth_ui/ifont.h"
#include "vulkan_buffer.h"
#include "vulkan_graphics.h"

extern "C" {
#include "font/geometry.h"
#include "font/outline.h"
}

#define NUMBER_OF_GLYPHS 96

namespace backend::vulkan {
    class Font : public moth_ui::IFont {
    public:
        static std::unique_ptr<Font> Load(char const* path, int size, Context& context, Graphics& graphics);
        virtual ~Font();

        int GetGlyphIndex(int charCode) const {
            auto const it = m_charCodeToIndex.find(charCode);
            if (std::end(m_charCodeToIndex) == it) {
                return 0;
            }
            return it->second;
        }

        moth_ui::IntVec2 GetGlyphSize(int charCode) const {
            int const gi = GetGlyphIndex(charCode);
            return m_glyphInfo[gi].Advance;
        }

        VkDescriptorSet GetVKDescriptorSet() const {
            return m_vkDescriptorSet;
        }

        uint32_t GetLineHeight() const { return m_lineHeight; }
        uint32_t GetStringWidth(std::string_view const& str) const;
        uint32_t GetColumnHeight(std::string const& str, uint32_t width) const;

    private:
        Font();
        void Init(Context& context, Graphics& graphics);

        struct GlyphInfo {
            moth_ui::IntVec2 Size;
            moth_ui::IntVec2 Advance;
            moth_ui::FloatVec2 UV0;
            moth_ui::FloatVec2 UV1;
        };

        uint32_t m_lineHeight;
        std::map<int, int> m_charCodeToIndex;
        std::vector<GlyphInfo> m_glyphInfo;
        std::unique_ptr<Image> m_glyphAtlas;

        VkDescriptorSet m_vkDescriptorSet;
        std::unique_ptr<Buffer> m_storageBuffer;
    };
}
