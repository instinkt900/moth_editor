#pragma once

class TexturePacker {
public:
    TexturePacker() = default;
    ~TexturePacker() = default;

    void Open() { m_open = true; }
    void Draw();

private:
    bool m_open = false;
};
