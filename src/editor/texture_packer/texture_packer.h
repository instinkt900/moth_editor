#pragma once

#include "moth_packer/packer.h"
#include "moth_graphics/graphics/iimage.h"

#include <array>
#include <filesystem>
#include <memory>
#include <vector>

class EditorLayer;

class TexturePacker {
public:
    explicit TexturePacker(EditorLayer& editorLayer);
    ~TexturePacker() = default;

    void Open() { m_open = true; }
    void Draw();

private:
    void DrawInputPanel();
    void DrawOutputPanel();

    // Input collection — each appends to m_inputImages, skipping duplicates
    void AddFile(std::filesystem::path const& path);
    void AddDirectory(std::filesystem::path const& dir, bool recursive);
    void AddLayout(std::filesystem::path const& layout);
    void AddLayoutDirectory(std::filesystem::path const& dir, bool recursive);

    void DoPack();
    void DoSave();

    EditorLayer& m_editorLayer;
    bool m_open = false;

    // ---- Input state ----
    std::vector<moth_packer::ImageDetails> m_inputImages;
    int m_selectedInput = -1;
    std::shared_ptr<moth_graphics::graphics::IImage> m_inputPreview;
    float m_inputZoom = 0.0f;  // 0 = fit-to-window; >0 = absolute scale

    // Pack options (persisted across pack/save)
    int m_minWidth   = 256;
    int m_minHeight  = 256;
    int m_maxWidth   = 4096;
    int m_maxHeight  = 4096;
    int m_padding    = 0;
    moth_packer::PaddingType  m_paddingType   = moth_packer::PaddingType::Color;
    uint32_t                  m_paddingColor  = 0x00000000;  // RRGGBBAA
    char m_paddingColorHex[9] = {"00000000"};  // persistent edit buffer for the hex InputText
    moth_packer::AtlasFormat  m_outputFormat  = moth_packer::AtlasFormat::PNG;
    int m_jpegQuality = 90;

    // Flipbook options
    bool m_flipbookMode = false;
    int  m_fps          = 12;
    moth_packer::LoopType m_loopType = moth_packer::LoopType::Loop;
    char m_clipName[64] = {"default"};

    // Last options used for pack — reused verbatim by Save (with outputPath/filename swapped)
    moth_packer::PackOptions m_lastPackOpts;

    // ---- Output state ----
    struct AtlasPreview {
        std::filesystem::path tempPath;
        std::shared_ptr<moth_graphics::graphics::IImage> image;
        int width  = 0;
        int height = 0;
        std::vector<std::string>          imageNames;
        std::vector<std::array<int, 4>>   imageRects; // {x, y, w, h}
    };

    std::vector<AtlasPreview> m_previewAtlases;
    bool m_hasPacked      = false;
    int  m_selectedOutput = -1;
    float m_outputZoom    = 0.0f;  // 0 = fit-to-window; >0 = absolute scale

    // Save destination
    char m_saveDir[1024]     = {};
    char m_saveFilename[256] = {"packed"};
};
