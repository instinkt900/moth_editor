#pragma once

#include "moth_ui/ifont.h"
#include "vulkan_buffer.h"
#include "vulkan_graphics.h"
#include "font/geometry.h"
#include "font/outline.h"

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
        Font();
        virtual ~Font();

    private:
        void Init(Context& context, Graphics& graphics);
        void Load(char const* path, Context& context);

        fd_Outline m_outlines[NUMBER_OF_GLYPHS];
        HostGlyphInfo m_glyphInfos[NUMBER_OF_GLYPHS];

        void* m_glyphData;
        uint32_t m_glyphDataSize = 0;
        uint32_t m_glyphInfoOffset = 0;
        uint32_t m_glyphInfoSize = 0;
        uint32_t m_glyphCellsOffset = 0;
        uint32_t m_glyphCellsSize = 0;
        uint32_t m_glyphPointsOffset = 0;
        uint32_t m_glyphPointsSize = 0;
        std::unique_ptr<Buffer> m_glyphBuffer;
        VkDescriptorSet m_fontDescriptorSet;
    };
}
