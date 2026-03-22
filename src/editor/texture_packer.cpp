#include "common.h"
#include "texture_packer.h"

#include "moth_packer/packer.h"

#include <nfd.h>
#include <spdlog/spdlog.h>

namespace {
    char s_layoutPathBuffer[1024];
    char s_outputPathBuffer[1024];
    int s_minWidth = 256;
    int s_minHeight = 256;
    int s_maxWidth = 1024;
    int s_maxHeight = 1024;
}

void TexturePacker::Draw() {
    if (m_open) {
        if (ImGui::Begin("Texture Packing", &m_open)) {
            ImGui::InputText("##layouts_path", s_layoutPathBuffer, 1023);
            ImGui::SameLine();
            if (ImGui::Button("...##layout")) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_PickFolder(currentPath.c_str(), &outPath);
                if (result == NFD_OKAY) {
                    strncpy(s_layoutPathBuffer, outPath, 1023);
                }
            }
            ImGui::SameLine();
            ImGui::Text("Layouts path");

            ImGui::InputText("##output_path", s_outputPathBuffer, 1023);
            ImGui::SameLine();
            if (ImGui::Button("...##output")) {
                auto const currentPath = std::filesystem::current_path().string();
                nfdchar_t* outPath = NULL;
                nfdresult_t result = NFD_PickFolder(currentPath.c_str(), &outPath);
                if (result == NFD_OKAY) {
                    strncpy(s_outputPathBuffer, outPath, 1023);
                }
            }
            ImGui::SameLine();
            ImGui::Text("Output path");

            ImGui::InputInt("Min Width", &s_minWidth);
            ImGui::InputInt("Min Height", &s_minHeight);
            ImGui::InputInt("Max Width", &s_maxWidth);
            ImGui::InputInt("Max Height", &s_maxHeight);

            if (ImGui::Button("Pack")) {
                std::vector<moth_packer::ImageDetails> images;
                moth_packer::CollectImagesFromLayoutsDir(s_layoutPathBuffer, true, images);
                if (!moth_packer::Pack(images, s_outputPathBuffer, "packed", true, false, s_minWidth, s_minHeight, s_maxWidth, s_maxHeight)) {
                    spdlog::error("Texture packing failed.");
                }
            }
        }
        ImGui::End();
    }
}
