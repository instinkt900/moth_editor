#include "common.h"
#include "editor_panel_fonts.h"
#include "moth_ui/context.h"
#include "../editor_layer.h"

#include <nfd.h>

EditorPanelFonts::EditorPanelFonts(EditorLayer& editorLayer)
    : m_editorLayer(editorLayer) {
}

void EditorPanelFonts::Draw() {
    if (!m_open) {
        return;
    }

    static char NameBuffer[1024] = { 0 };

    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(360, 260), ImVec2(FLT_MAX, FLT_MAX));
    if (!ImGui::Begin("Fonts", &m_open)) {
        ImGui::End();
        return;
    }

    auto& fontFactory = m_editorLayer.GetContext().GetFontFactory();
    auto fontNames = fontFactory.GetFontNameList();

    // ── Project row ──────────────────────────────────────────────────────────
    ImGui::Text("Font project:");
    ImGui::SameLine();
    if (ImGui::Button("Load...")) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        if (NFD_OpenDialog("json", currentPath.c_str(), &outPath) == NFD_OKAY) {
            fontFactory.LoadProject(std::filesystem::path(outPath));
            fontNames = fontFactory.GetFontNameList();
            m_editorLayer.Rebuild();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save...")) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        if (NFD_SaveDialog("json", currentPath.c_str(), &outPath) == NFD_OKAY) {
            std::filesystem::path filePath = outPath;
            if (!filePath.has_extension()) {
                filePath.replace_extension(".json");
            }
            fontFactory.SaveProject(filePath);
        }
    }
    ImGui::Separator();

    // ── List + side buttons ──────────────────────────────────────────────────
    // Reserve bottom area: path label + path text + separator + close button
    float const bottomReserve = (ImGui::GetFrameHeightWithSpacing() * 3.0f) + ImGui::GetStyle().ItemSpacing.y;
    float const listHeight = ImGui::GetContentRegionAvail().y - bottomReserve;

    float const sideButtonW = 110.0f;
    float const listW = ImGui::GetContentRegionAvail().x - sideButtonW - ImGui::GetStyle().ItemSpacing.x;

    ImGui::PushID(this);
    if (ImGui::BeginListBox("##font_list", ImVec2(listW, listHeight))) {
        for (int i = 0; i < static_cast<int>(fontNames.size()); ++i) {
            bool const selected = m_selectedIndex == i;
            if (ImGui::Selectable(fontNames[i].c_str(), selected)) {
                m_selectedIndex = i;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::BeginGroup();
    if (ImGui::Button("Add Font", ImVec2(sideButtonW, 0))) {
        auto const currentPath = std::filesystem::current_path().string();
        nfdchar_t* outPath = NULL;
        if (NFD_OpenDialog("ttf", currentPath.c_str(), &outPath) == NFD_OKAY) {
            m_pendingFontPath = outPath;
            NameBuffer[0] = 0;
            ImGui::OpenPopup("Name Font");
        }
    }
    bool const canRemove = m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(fontNames.size());
    if (!canRemove) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Remove", ImVec2(sideButtonW, 0))) {
        fontFactory.RemoveFont(fontNames[m_selectedIndex]);
        fontNames = fontFactory.GetFontNameList();
        m_selectedIndex = std::min(m_selectedIndex, static_cast<int>(fontNames.size()) - 1);
    }
    if (!canRemove) {
        ImGui::EndDisabled();
    }
    ImGui::EndGroup();

    // ── Path display ─────────────────────────────────────────────────────────
    ImGui::Separator();
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(fontNames.size())) {
        std::string const path = fontFactory.GetFontPath(fontNames[m_selectedIndex]).string();
        ImGui::TextUnformatted("Path:");
        ImGui::SameLine();
        ImGui::TextWrapped("%s", path.c_str());
    } else {
        ImGui::TextDisabled("No font selected.");
    }

    // ── Close button (bottom-right) ───────────────────────────────────────────
    float const closeW = 80.0f;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - closeW);
    if (ImGui::Button("Close", ImVec2(closeW, 0))) {
        m_open = false;
    }

    // ── Name Font popup ───────────────────────────────────────────────────────
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(FLT_MAX, FLT_MAX));
    if (ImGui::BeginPopupModal("Name Font", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("File:  %s", m_pendingFontPath.filename().string().c_str());
        ImGui::Separator();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##name", NameBuffer, 1024);
        ImGui::Spacing();
        float const btnW = ImGui::GetFontSize() * 7.0f;
        if (ImGui::Button("OK", ImVec2(btnW, 0))) {
            fontFactory.AddFont(NameBuffer, m_pendingFontPath);
            m_editorLayer.Rebuild();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(btnW, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
