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
    struct CellInfo {
        uint32_t point_offset;
        uint32_t cell_offset;
        uint32_t cell_count_x;
        uint32_t cell_count_y;
    };

    struct HostGlyphInfo {
        fd_Rect bbox;
        float advance;
    };

    struct DeviceGlyphInfo {
        fd_Rect bbox;
        CellInfo cell_info;
    };

    class Font : public moth_ui::IFont {
    public:
        static std::unique_ptr<Font> Load(char const* path, Context& context, Graphics& graphics);
        virtual ~Font();

        const HostGlyphInfo* GetGlyphInfo(int index) const { return &m_glyphInfos[index]; }
        VkDescriptorSet GetDescriptorSet() const { return m_fontDescriptorSet; }

    private:
        Font();
        void Init(Context& context, Graphics& graphics);

        fd_Outline m_outlines[NUMBER_OF_GLYPHS];
        HostGlyphInfo m_glyphInfos[NUMBER_OF_GLYPHS];

        uint32_t m_glyphInfoOffset = 0;
        uint32_t m_glyphInfoSize = 0;
        uint32_t m_glyphCellsOffset = 0;
        uint32_t m_glyphCellsSize = 0;
        uint32_t m_glyphPointsOffset = 0;
        uint32_t m_glyphPointsSize = 0;

        VkDescriptorSet m_fontDescriptorSet;

        std::unique_ptr<Buffer> m_storageBuffer;
    };
}
