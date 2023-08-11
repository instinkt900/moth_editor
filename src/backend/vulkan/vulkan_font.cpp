#include "common.h"
#include "vulkan_font.h"
#include "stb_rect_pack.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

    int NextPowerOf2(int value) {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value++;
        return value;
    }

    moth_ui::IntVec2 FindOptimalDimensions(std::vector<stbrp_node>& nodes, std::vector<stbrp_rect>& rects, moth_ui::IntVec2 const& minPack, moth_ui::IntVec2 const& maxPack) {
        // collect some info about all the rects
        int minWidth = std::numeric_limits<int>::max();
        int minHeight = std::numeric_limits<int>::max();
        int maxWidth = std::numeric_limits<int>::min();
        int maxHeight = std::numeric_limits<int>::min();
        int totalArea = 0;
        int totalWidth = 0;
        int totalHeight = 0;

        for (auto&& rect : rects) {
            minWidth = std::min(minWidth, rect.w);
            minHeight = std::min(minHeight, rect.h);
            maxWidth = std::max(maxWidth, rect.w);
            maxHeight = std::max(maxHeight, rect.h);
            totalWidth += rect.w;
            totalHeight += rect.h;
            totalArea += rect.w * rect.h;
        }

        struct PackTest {
            moth_ui::IntVec2 m_dimensions;
            float m_ratio;
        };

        int const minDimX = NextPowerOf2(minPack.x);
        int const minDimY = NextPowerOf2(minPack.y);
        int const maxDimX = NextPowerOf2(maxPack.x);
        int const maxDimY = NextPowerOf2(maxPack.y);

        std::vector<PackTest> testDimensions;
        int curWidth = minDimX;
        while (curWidth <= maxDimX) {
            int curHeight = minDimY;
            while (curHeight <= maxDimY) {
                int const curArea = curWidth * curHeight;
                if (curArea > totalArea) {
                    PackTest info;
                    info.m_dimensions = moth_ui::IntVec2{ curWidth, curHeight };
                    info.m_ratio = 0;
                    testDimensions.push_back(info);
                }
                curHeight *= 2;
            }
            curWidth *= 2;
        }

        for (auto&& testDim : testDimensions) {
            stbrp_context stbContext;
            stbrp_init_target(&stbContext, testDim.m_dimensions.x, testDim.m_dimensions.y, nodes.data(), static_cast<int>(nodes.size()));
            auto const allPacked = stbrp_pack_rects(&stbContext, rects.data(), static_cast<int>(rects.size()));
            if (allPacked) {
                float testArea = static_cast<float>(testDim.m_dimensions.x * testDim.m_dimensions.y);
                testDim.m_ratio = totalArea / testArea;
            }
        }

        std::sort(std::begin(testDimensions), std::end(testDimensions), [](auto const& a, auto const& b) { return b.m_ratio < a.m_ratio; });
        return std::begin(testDimensions)->m_dimensions;
    }

    std::unique_ptr<Font> Font::Load(char const* path, int size, Context& context, Graphics& graphics) {
        FT_Library library;
        FT_CHECK(FT_Init_FreeType(&library));

        FT_Face face;
        if (FT_New_Face(library, path, 0, &face) != 0) {
            return nullptr;
        }

        FT_CHECK(FT_Set_Pixel_Sizes(face, 0, size));

        uint32_t total_points = 0;
        uint32_t total_cells = 0;

        std::vector<stbrp_rect> stbRects;
        int minGlyphWidth, maxGlyphWidth;
        int minGlyphHeight, maxGlyphHeight;
        int fullWidth = 0;
        int fullHeight = 0;

        minGlyphWidth = minGlyphHeight = std::numeric_limits<int>::max();
        maxGlyphWidth = maxGlyphHeight = 0;

        std::unique_ptr<Font> font(new Font);
        std::vector<int> charcodes;

        // first we iterate through all the glyphs in the font. measuring them and preparing
        // the rects for the stb packer
        FT_ULong charcode;
        FT_UInt gindex;
        charcode = FT_Get_First_Char(face, &gindex);
        while (gindex != 0) {
            FT_CHECK(FT_Load_Glyph(face, gindex, FT_LOAD_NO_HINTING));

            const int glyphWidth = face->glyph->bitmap.width;
            const int glyphHeight = face->glyph->bitmap.rows;

            minGlyphWidth = std::min(minGlyphWidth, glyphWidth);
            minGlyphHeight = std::min(minGlyphHeight, glyphHeight);
            maxGlyphWidth = std::max(maxGlyphWidth, glyphWidth);
            maxGlyphHeight = std::max(maxGlyphHeight, glyphHeight);
            fullWidth += glyphWidth;
            fullHeight += glyphHeight;

            stbrp_rect r;
            r.id = gindex;
            r.w = glyphWidth;
            r.h = glyphHeight;

            stbRects.push_back(r);
            // want to store a list of charcodes in index order so we can
            // map charcode to rect.id/glyph index later
            charcodes.push_back(charcode);

            charcode = FT_Get_Next_Char(face, charcode, &gindex);
        }

        // now we use the stb packer to optimally pack them in an arrangement
        std::vector<stbrp_node> stbNodes(fullWidth * 2);
        auto const packDim = FindOptimalDimensions(stbNodes, stbRects, { 1, 1 }, { fullWidth, fullHeight });
        stbrp_context stbContext;
        stbrp_init_target(&stbContext, packDim.x, packDim.y, stbNodes.data(), static_cast<int>(stbNodes.size()));
        stbrp_pack_rects(&stbContext, stbRects.data(), static_cast<int>(stbRects.size()));

        const int dataStride = packDim.x * 4;
        std::vector<unsigned char> packData(dataStride * packDim.y);
        moth_ui::FloatVec2 const texSize(static_cast<float>(packDim.x), static_cast<float>(packDim.y));

        // now we render the glyph into some texture data and store the position
        // and size of the glyph for the charcode
        for (auto&& rect : stbRects) {
            // render glyph
            FT_GlyphSlot glyphSlot = face->glyph;
            FT_CHECK(FT_Load_Glyph(face, rect.id, FT_LOAD_NO_HINTING));
            FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_NORMAL);
            int const glyphStride = face->glyph->bitmap.pitch;
            int const glyphPosX = rect.x;
            int const glyphPosY = rect.y;
            int const glyphWidth = rect.w;
            int const glyphHeight = rect.h;
            for (int y = 0; y < glyphHeight; ++y) {
                for (int x = 0; x < glyphWidth; ++x) {
                    unsigned char alpha = face->glyph->bitmap.buffer[x + y * glyphStride];
                    packData[((glyphPosX + x) * 4) + ((glyphPosY + y) * dataStride) + 0] = alpha;
                    packData[((glyphPosX + x) * 4) + ((glyphPosY + y) * dataStride) + 1] = alpha;
                    packData[((glyphPosX + x) * 4) + ((glyphPosY + y) * dataStride) + 2] = alpha;
                    packData[((glyphPosX + x) * 4) + ((glyphPosY + y) * dataStride) + 3] = alpha;
                }
            }

            // store info
            int const glyphIndex = static_cast<int>(font->m_glyphInfo.size());
            int const glyphCode = charcodes[glyphIndex];
            moth_ui::FloatVec2 const pos0(static_cast<float>(glyphPosX), static_cast<float>(glyphPosY));
            moth_ui::FloatVec2 const pos1(static_cast<float>(glyphPosX + glyphWidth), static_cast<float>(glyphPosY + glyphHeight));
            moth_ui::FloatVec2 const uv0 = pos0 / texSize;
            moth_ui::FloatVec2 const uv1 = pos1 / texSize;
            // https://stackoverflow.com/questions/66265216/how-is-freetype-calculating-advance
            font->m_glyphInfo.push_back({ { glyphWidth, glyphHeight }, { glyphSlot->advance.x / 64, glyphSlot->advance.y / 64 }, uv0, uv1 });
            font->m_charCodeToIndex.insert(std::make_pair(glyphCode, glyphIndex));
        }

        font->m_glyphAtlas = Image::FromRGBA(context, packDim.x, packDim.y, packData.data());

        // stbi_write_png("font.png", packDim.x, packDim.y, 4, packData.data(), dataStride);

        // for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {
        //     char c = ' ' + i;
        //     //printf("%c", c);

        //    fd_Outline* o = &font->m_outlines[i];
        //    HostGlyphInfo* hgi = &font->m_glyphInfos[i];

        //    FT_UInt glyph_index = FT_Get_Char_Index(face, c);
        //    FT_CHECK(FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING));

        //    fd_outline_convert(&face->glyph->outline, o, c);

        //    hgi->bbox = o->bbox;
        //    hgi->advance = face->glyph->metrics.horiAdvance / 64.0f;


        //    total_points += o->num_of_points;
        //    total_cells += o->cell_count_x * o->cell_count_y;
        //}

        // font->m_glyphInfoSize = sizeof(DeviceGlyphInfo) * NUMBER_OF_GLYPHS;
        // font->m_glyphCellsSize = sizeof(uint32_t) * total_cells;
        // font->m_glyphPointsSize = sizeof(vec2) * total_points;

        // uint32_t alignment = static_cast<uint32_t>(context.m_vkDeviceProperties.limits.minStorageBufferOffsetAlignment);
        // font->m_glyphInfoOffset = 0;
        // font->m_glyphCellsOffset = align_uint32(font->m_glyphInfoSize, alignment);
        // font->m_glyphPointsOffset = align_uint32(font->m_glyphInfoSize + font->m_glyphCellsSize, alignment);

        // uint32_t glyphDataSize = font->m_glyphPointsOffset + font->m_glyphPointsSize;
        // std::vector<char> glyphData(glyphDataSize);

        // DeviceGlyphInfo* device_glyph_infos = (DeviceGlyphInfo*)(glyphData.data() + font->m_glyphInfoOffset);
        // uint32_t* cells = (uint32_t*)(glyphData.data() + font->m_glyphCellsOffset);
        // vec2* points = (vec2*)(glyphData.data() + font->m_glyphPointsOffset);

        // uint32_t point_offset = 0;
        // uint32_t cell_offset = 0;

        // for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++) {
        //     fd_Outline* o = &font->m_outlines[i];
        //     DeviceGlyphInfo* dgi = &device_glyph_infos[i];

        //    dgi->cell_info.cell_count_x = o->cell_count_x;
        //    dgi->cell_info.cell_count_y = o->cell_count_y;
        //    dgi->cell_info.point_offset = point_offset;
        //    dgi->cell_info.cell_offset = cell_offset;
        //    dgi->bbox = o->bbox;

        //    uint32_t cell_count = o->cell_count_x * o->cell_count_y;
        //    memcpy(cells + cell_offset, o->cells, sizeof(uint32_t) * cell_count);
        //    memcpy(points + point_offset, o->points, sizeof(vec2) * o->num_of_points);

        //    //fd_outline_u16_points(o, &dgi->cbox, points + point_offset);

        //    point_offset += o->num_of_points;
        //    cell_offset += cell_count;
        //}

        // assert(point_offset == total_points);
        // assert(cell_offset == total_cells);

        // for (uint32_t i = 0; i < NUMBER_OF_GLYPHS; i++)
        //     fd_outline_destroy(&font->m_outlines[i]);

        // FT_CHECK(FT_Done_Face(face));
        // FT_CHECK(FT_Done_FreeType(library));

        ////printf("\n");
        ////printf("Avarage glyph size: %d bytes\n", r->glyph_data_size / NUMBER_OF_GLYPHS);
        ////printf("    points size: %d bytes\n", r->glyph_points_size / NUMBER_OF_GLYPHS);
        ////printf("    cells size: %d bytes\n", r->glyph_cells_size / NUMBER_OF_GLYPHS);

        int const dataSize = static_cast<int>(font->m_glyphInfo.size() * sizeof(GlyphInfo));
        font->m_storageBuffer = std::make_unique<Buffer>(context, dataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        std::unique_ptr<Buffer> staging = std::make_unique<Buffer>(context, dataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        unsigned char* stagingPtr = static_cast<unsigned char*>(staging->Map());
        memcpy(stagingPtr, font->m_glyphInfo.data(), dataSize);
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
        CHECK_VK_RESULT(vkAllocateDescriptorSets(context.m_vkDevice, &alloc_info, &m_vkDescriptorSet));

        VkDescriptorBufferInfo glyph_info[1] = {};
        glyph_info[0].buffer = m_storageBuffer->GetVKBuffer();
        glyph_info[0].offset = 0;
        glyph_info[0].range = m_glyphInfo.size() * sizeof(GlyphInfo);

        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = m_glyphAtlas->GetVkSampler();
        desc_image[0].imageView = m_glyphAtlas->GetVkView();
        desc_image[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write_desc[3] = {};

        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = m_vkDescriptorSet;
        write_desc[0].dstBinding = 0;
        write_desc[0].dstArrayElement = 0;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_desc[0].pBufferInfo = glyph_info;

        write_desc[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[1].dstSet = m_vkDescriptorSet;
        write_desc[1].dstBinding = 1;
        write_desc[1].descriptorCount = 1;
        write_desc[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[1].pImageInfo = desc_image;

        vkUpdateDescriptorSets(context.m_vkDevice, 2, write_desc, 0, nullptr);
    }
}
