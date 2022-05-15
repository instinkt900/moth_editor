#pragma once

class TexturePacker {
public:
    TexturePacker();
    ~TexturePacker();

    void Open() {
        m_open = true;
    }
    void Draw();

private:
    bool m_open = false;
    TextureRef m_outputTexture;
    int m_textureWidth;
    int m_textureHeight;

    void Pack(std::filesystem::path const& inputPath, std::filesystem::path const& outputPath, int minWidth, int minHeight, int maxWidth, int maxHeight);
};
