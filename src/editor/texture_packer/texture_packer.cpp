#include "common.h"
#include "texture_packer.h"
#include "editor/editor_layer.h"

#include <nfd.h>

TexturePacker::TexturePacker(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

void TexturePacker::Draw() {
    if (!m_open) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(1000, 650), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Texture Packer", &m_open)) {
        if (ImGui::BeginTable("##tp_layout", 2,
                ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
            ImGui::TableSetupColumn("Input",  ImGuiTableColumnFlags_WidthStretch, 0.5f);
            ImGui::TableSetupColumn("Output", ImGuiTableColumnFlags_WidthStretch, 0.5f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::BeginChild("##tp_input", ImVec2(0, 0), ImGuiChildFlags_None);
            DrawInputPanel();
            ImGui::EndChild();

            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("##tp_output", ImVec2(0, 0), ImGuiChildFlags_None);
            DrawOutputPanel();
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::End();
}

// ---- Input collection helpers -----------------------------------------------

void TexturePacker::AddFile(std::filesystem::path const& path) {
    std::vector<moth::packer::ImageDetails> collected;
    // CollectImagesFromFile expects a text file listing paths; for a single image
    // file we build a one-entry ImageDetails manually after validating it.
    // Re-use the glob path to stay consistent with dedup logic.
    moth::packer::CollectImagesFromGlob(path.string(), collected);
    for (auto& img : collected) {
        auto const dup = [&](moth::packer::ImageDetails const& d) { return d.path == img.path; };
        if (std::find_if(m_inputImages.begin(), m_inputImages.end(), dup) == m_inputImages.end()) {
            m_inputImages.push_back(std::move(img));
        }
    }
}

void TexturePacker::AddDirectory(std::filesystem::path const& dir, bool recursive) {
    moth::packer::CollectImagesFromDir(dir, recursive, m_inputImages);
}

void TexturePacker::AddLayout(std::filesystem::path const& layout) {
    moth::packer::CollectImagesFromLayout(layout, m_inputImages);
}

void TexturePacker::AddLayoutDirectory(std::filesystem::path const& dir, bool recursive) {
    moth::packer::CollectImagesFromLayoutsDir(dir, recursive, m_inputImages);
}
