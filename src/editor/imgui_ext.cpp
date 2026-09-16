#include "common.h"
#include "imgui_ext.h"
#include "imgui_internal.h"
#include "moth/ui/utils/vector.h"
#include "moth/ui/graphics/iimage.h"
#include "moth/bridge/moth_image.h"
#include "moth/graphics/graphics/itexture.h"
#include "moth/graphics/platform/imgui_context.h"

namespace {
    moth::gfx::platform::ImGuiContext* s_imguiContext = nullptr;

    // Reproduces the source-rect UV remapping that moth_graphics' Image::DrawImGui did,
    // before the toolkit moved texture drawing off Image and onto the ImGui context.
    void DrawImage(moth::gfx::Image const& image, moth::ui::IntVec2 const& size,
                   moth::ui::FloatVec2 const& uv0, moth::ui::FloatVec2 const& uv1) {
        auto const& texture = image.GetTexture();
        if (s_imguiContext == nullptr || !texture) {
            return;
        }
        auto const& source = image.GetSourceRect();
        auto const texW = static_cast<float>(texture->GetWidth());
        auto const texH = static_cast<float>(texture->GetHeight());
        if (texW <= 0.0f || texH <= 0.0f) {
            return;
        }
        auto const srcW = static_cast<float>(source.w());
        auto const srcH = static_cast<float>(source.h());
        auto const offX = static_cast<float>(source.topLeft.x);
        auto const offY = static_cast<float>(source.topLeft.y);
        s_imguiContext->Image(*texture, size,
                              { (offX + (uv0.x * srcW)) / texW, (offY + (uv0.y * srcH)) / texH },
                              { (offX + (uv1.x * srcW)) / texW, (offY + (uv1.y * srcH)) / texH });
    }
}

namespace imgui_ext {
    using namespace moth::ui;

    bool InputString(char const* label, std::string* str) {
        static size_t const BufferSize = 1024;
        static char buffer[BufferSize];
        strncpy(buffer, str->c_str(), BufferSize - 1);
        if (ImGui::InputText(label, buffer, BufferSize - 1)) {
            *str = buffer;
            return true;
        }
        return false;
    }

    bool InputKeyframeValue(char const* label, KeyframeValue* value) {
        return ImGui::InputFloat(label, value);
    }

    void InputIntVec2(char const* label, IntVec2* vec) {
        auto const inputWidth = ImMax(1.0f, (ImGui::CalcItemWidth() - 20) / 2.0f);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::PushID(&vec->x);
        ImGui::InputInt("", &vec->x, 0);
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(20);
        ImGui::Text("x");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::PushID(&vec->y);
        ImGui::InputInt("", &vec->y, 0);
        ImGui::PopID();
        ImGui::SameLine(0, 4);
        ImGui::Text("%s", label);
    }

    void InputFloatVec2(char const* label, FloatVec2* vec) {
        auto const inputWidth = ImMax(1.0f, (ImGui::CalcItemWidth() - 20) / 2.0f);
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::PushID(&vec->x);
        ImGui::InputFloat("", &vec->x, 0);
        ImGui::PopID();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(20);
        ImGui::Text("x");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(inputWidth);
        ImGui::PushID(&vec->y);
        ImGui::InputFloat("", &vec->y, 0);
        ImGui::PopID();
        ImGui::SameLine(0, 4);
        ImGui::Text("%s", label);
    }

    void SetImGuiContext(moth::gfx::platform::ImGuiContext* context) {
        s_imguiContext = context;
    }

    void Image(moth::gfx::Image const& image, int width, int height) {
        DrawImage(image, { width, height }, { 0.0f, 0.0f }, { 1.0f, 1.0f });
    }

    void Image(moth::gfx::Image const& image, int width, int height,
               moth::ui::FloatVec2 const& uv0, moth::ui::FloatVec2 const& uv1) {
        DrawImage(image, { width, height }, uv0, uv1);
    }

    void Image(moth::ui::IImage const* image, int width, int height) {
        if (image != nullptr) {
            auto const* mothImage = dynamic_cast<moth::bridge::MothImage const*>(image);
            if (mothImage != nullptr) {
                Image(mothImage->GetImage(), width, height);
            }
        }
    }
}
