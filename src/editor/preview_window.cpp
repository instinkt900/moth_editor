#include "common.h"
#include "preview_window.h"
#include "app.h"
#include "moth_ui/node.h"
#include "moth_ui/group.h"
#include "moth_ui/layout/layout_entity_group.h"
#include "moth_ui/animation_clip.h"

extern App* g_App;

PreviewWindow::PreviewWindow() {
}


bool PreviewWindow::OnEvent(moth_ui::Event const& event) {
    return false;
}

void PreviewWindow::Update(uint32_t ticks) {
    if (m_root) {
        m_root->Update(ticks);
    }
}

void PreviewWindow::Draw() {
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
        moth_ui::IntVec2 const previewSize{ static_cast<int>(windowRegionMax.x), static_cast<int>(windowRegionMax.y) };
        UpdateRenderSurface(previewSize);
        SDL_SetRenderTarget(g_App->GetRenderer(), m_renderSurface.get());

        moth_ui::IntRect displayRect;
        displayRect.topLeft = { 0, 0 };
        displayRect.bottomRight = previewSize;
        m_root->SetScreenRect(displayRect);
        m_root->Draw();

        SDL_SetRenderTarget(g_App->GetRenderer(), nullptr);

        ImGui::Image(m_renderSurface.get(), ImVec2(static_cast<float>(previewSize.x), static_cast<float>(previewSize.y)));
    }
}

void PreviewWindow::Refresh(std::shared_ptr<moth_ui::LayoutEntityGroup> layout) {

    layout->RefreshAnimationTimings();

    auto group = std::make_unique<moth_ui::Group>(layout);
    auto const& clips = layout->m_clips;
    m_clipNames.clear();
    for (auto&& clip : clips) {
        m_clipNames.push_back(clip->m_name);
    }
    m_root = std::move(group);
}

void PreviewWindow::UpdateRenderSurface(moth_ui::IntVec2 surfaceSize) {
    if (!m_renderSurface || m_currentSurfaceSize != surfaceSize) {
        m_currentSurfaceSize = surfaceSize;
        m_renderSurface = CreateTextureRef(SDL_CreateTexture(g_App->GetRenderer(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, m_currentSurfaceSize.x, m_currentSurfaceSize.y));
    }
}
