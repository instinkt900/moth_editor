#include "common.h"
#include "imgui_ext.h"
#include "imgui_internal.h"
#include "moth_ui/utils/vector.h"
#include "moth_ui/iimage.h"

namespace imgui_ext {
    using namespace moth_ui;

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

    void Image(moth_ui::IImage const* image, int width, int height) {
        if (image) {
            image->ImGui({ width, height });
        } else {
            //ImGui::Image(nullptr, ImVec2(static_cast<float>(width), static_cast<float>(height)));
        }
    }
}
