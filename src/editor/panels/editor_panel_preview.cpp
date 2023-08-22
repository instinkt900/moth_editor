#include "common.h"
#include "editor_panel_preview.h"
#include "moth_ui/layout/layout_entity_group.h"
#include "moth_ui/animation_clip.h"
#include "moth_ui/group.h"
#include "moth_ui/layout/layout.h"
#include "moth_ui/itarget.h"
#include "../editor_layer.h"
#include "editor_application.h"

EditorPanelPreview::EditorPanelPreview(EditorLayer& editorLayer, bool visible)
    : EditorPanel(editorLayer, "Preview", visible, true) {
}

void EditorPanelPreview::SetLayout(std::shared_ptr<moth_ui::Layout> layout) {
    auto group = std::make_unique<moth_ui::Group>(layout);
    auto const& clips = layout->m_clips;
    m_clipNames.clear();
    for (auto&& clip : clips) {
        m_clipNames.push_back(clip->m_name);
    }
    m_root = std::move(group);
}

bool EditorPanelPreview::OnEvent(moth_ui::Event const& event) {
    return false;
}

void EditorPanelPreview::Update(uint32_t ticks) {
    if (m_visible && !m_wasVisible) {
        SetLayout(m_editorLayer.GetCurrentLayout());
    }

    m_wasVisible = m_visible;

    if (m_visible && m_root) {
        m_root->Update(ticks);
    }
}

void EditorPanelPreview::DrawContents() {
    if (m_root) {
        ImGui::Text("Animation");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::BeginCombo("##ClipName", m_selectedClip.c_str())) {
            for (auto&& clipName : m_clipNames) {
                bool selected = clipName == m_selectedClip;
                if (ImGui::Selectable(clipName.c_str(), &selected)) {
                    m_selectedClip = clipName;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Play")) {
            if (!m_selectedClip.empty()) {
                m_root->SetAnimation(m_selectedClip);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            m_root->StopAnimation();
        }

        auto const windowRegionMax = ImGui::GetContentRegionAvail();
        if (windowRegionMax.x > 0 && windowRegionMax.y > 0) {
            moth_ui::IntVec2 const previewSize{ static_cast<int>(windowRegionMax.x), static_cast<int>(windowRegionMax.y) };
            UpdateRenderSurface(previewSize);
            g_App->GetGraphics().SetTarget(m_renderSurface.get());

            moth_ui::IntRect displayRect;
            displayRect.topLeft = { 0, 0 };
            displayRect.bottomRight = previewSize;
            m_root->SetScreenRect(displayRect);
            m_root->Draw();

            g_App->GetGraphics().SetTarget(nullptr);

            imgui_ext::Image(m_renderSurface->GetImage(), previewSize.x, previewSize.y);
        }
    }
}

void EditorPanelPreview::UpdateRenderSurface(moth_ui::IntVec2 surfaceSize) {
    if (!m_renderSurface || m_currentSurfaceSize != surfaceSize) {
        m_currentSurfaceSize = surfaceSize;
        m_renderSurface = g_App->GetGraphics().CreateTarget(m_currentSurfaceSize.x, m_currentSurfaceSize.y);
    }
}
