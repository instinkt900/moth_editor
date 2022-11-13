#include "common.h"
#include "vulkan_font.h"

#define FT_CHECK(r)         \
    {                       \
        FT_Error err = (r); \
        assert(!err);       \
    }                       \
    while (0)

static uint32_t align_uint32(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

namespace backend::vulkan {
    Font::Font() {
    }

    Font::~Font() {
    }

    std::unique_ptr<Font> Font::Load(char const* path, Context& context, Graphics& graphics) {
        FT_Library library;
        FT_CHECK(FT_Init_FreeType(&library));

        FT_Face face;
        if (FT_New_Face(library, path, 0, &face) != 0) {
            return nullptr;
        }

        FT_CHECK(FT_Set_Char_Size(face, 0, 1000 * 64, 96, 96));

        uint32_t total_points = 0;
        uint32_t total_cells = 0;

        std::unique_ptr<Font> font(new Font);

        for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {
            char c = ' ' + i;
            //printf("%c", c);

            fd_Outline* o = &font->m_outlines[i];
            HostGlyphInfo* hgi = &font->m_glyphInfos[i];

            FT_UInt glyph_index = FT_Get_Char_Index(face, c);
            FT_CHECK(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING));

            fd_outline_convert(&face->glyph->outline, o, c);

            hgi->bbox = o->bbox;
            hgi->advance = face->glyph->metrics.horiAdvance / 64.0f;


            total_points += o->num_of_points;
            total_cells += o->cell_count_x * o->cell_count_y;
        }

        font->m_glyphInfoSize = sizeof(DeviceGlyphInfo) * NUMBER_OF_GLYPHS;
        font->m_glyphCellsSize = sizeof(uint32_t) * total_cells;
        font->m_glyphPointsSize = sizeof(vec2) * total_points;

        uint32_t alignment = static_cast<uint32_t>(context.m_vkDeviceProperties.limits.minStorageBufferOffsetAlignment);
        font->m_glyphInfoOffset = 0;
        font->m_glyphCellsOffset = align_uint32(font->m_glyphInfoSize, alignment);
        font->m_glyphPointsOffset = align_uint32(font->m_glyphInfoSize + font->m_glyphCellsSize, alignment);

        uint32_t glyphDataSize = font->m_glyphPointsOffset + font->m_glyphPointsSize;
        std::vector<char> glyphData(glyphDataSize);

        DeviceGlyphInfo* device_glyph_infos = (DeviceGlyphInfo*)(glyphData.data() + font->m_glyphInfoOffset);
        uint32_t* cells = (uint32_t*)(glyphData.data() + font->m_glyphCellsOffset);
        vec2* points = (vec2*)(glyphData.data() + font->m_glyphPointsOffset);

        uint32_t point_offset = 0;
        uint32_t cell_offset = 0;

        for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {
            fd_Outline* o = &font->m_outlines[i];
            DeviceGlyphInfo* dgi = &device_glyph_infos[i];

            dgi->cell_info.cell_count_x = o->cell_count_x;
            dgi->cell_info.cell_count_y = o->cell_count_y;
            dgi->cell_info.point_offset = point_offset;
            dgi->cell_info.cell_offset = cell_offset;
            dgi->bbox = o->bbox;

            uint32_t cell_count = o->cell_count_x * o->cell_count_y;
            memcpy(cells + cell_offset, o->cells, sizeof(uint32_t) * cell_count);
            memcpy(points + point_offset, o->points, sizeof(vec2) * o->num_of_points);

            //fd_outline_u16_points(o, &dgi->cbox, points + point_offset);

            point_offset += o->num_of_points;
            cell_offset += cell_count;
        }

        assert(point_offset == total_points);
        assert(cell_offset == total_cells);

        for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++)
            fd_outline_destroy(&font->m_outlines[i]);

        FT_CHECK(FT_Done_Face(face));
        FT_CHECK(FT_Done_FreeType(library));

        //printf("\n");
        //printf("Avarage glyph size: %d bytes\n", r->glyph_data_size / NUMBER_OF_GLYPHS);
        //printf("    points size: %d bytes\n", r->glyph_points_size / NUMBER_OF_GLYPHS);
        //printf("    cells size: %d bytes\n", r->glyph_cells_size / NUMBER_OF_GLYPHS);

        font->m_storageBuffer = std::make_unique<Buffer>(context, glyphDataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        std::unique_ptr<Buffer> staging = std::make_unique<Buffer>(context, glyphDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        void* stagingPtr = staging->Map();
        memcpy(stagingPtr, glyphData.data(), glyphDataSize);
        staging->Unmap();
        font->m_storageBuffer->Copy(*staging);

        font->Init(context, graphics);

        return font;
    }

    void Font::Init(Context& context, Graphics& graphics) {
        Shader& fontShader = graphics.GetFontShader();

        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = fontShader.m_descriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &fontShader.m_descriptorSetLayout;
        CHECK_VK_RESULT(vkAllocateDescriptorSets(context.m_vkDevice, &alloc_info, &m_fontDescriptorSet));

        VkDescriptorBufferInfo glyph_info[1] = {};
        glyph_info[0].buffer = m_storageBuffer->GetVKBuffer();
        glyph_info[0].offset = m_glyphInfoOffset;
        glyph_info[0].range = m_glyphInfoSize;

        VkDescriptorBufferInfo cells_info[1] = {};
        cells_info[0].buffer = m_storageBuffer->GetVKBuffer();
        cells_info[0].offset = m_glyphCellsOffset;
        cells_info[0].range = m_glyphCellsSize;

        VkDescriptorBufferInfo points_info[1] = {};
        points_info[0].buffer = m_storageBuffer->GetVKBuffer();
        points_info[0].offset = m_glyphPointsOffset;
        points_info[0].range = m_glyphPointsSize;

        VkWriteDescriptorSet write_desc[3] = {};

        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_fontDescriptorSet;
        write_desc[0].dstBinding = 0;
        write_desc[0].dstArrayElement = 0;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_desc[0].pBufferInfo = glyph_info;

        write_desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[1].dstSet = m_fontDescriptorSet;
        write_desc[1].dstBinding = 1;
        write_desc[1].dstArrayElement = 0;
        write_desc[1].descriptorCount = 1;
        write_desc[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_desc[1].pBufferInfo = cells_info;

        write_desc[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[2].dstSet = m_fontDescriptorSet;
        write_desc[2].dstBinding = 2;
        write_desc[2].dstArrayElement = 0;
        write_desc[2].descriptorCount = 1;
        write_desc[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_desc[2].pBufferInfo = points_info;

        vkUpdateDescriptorSets(context.m_vkDevice, 3, write_desc, 0, nullptr);
    }
}
