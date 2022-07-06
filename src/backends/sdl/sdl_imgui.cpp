#include "common.h"
#include "smart_sdl.h"
#include "sdl_image.h"
#include "moth_ui/iimage.h"

namespace imgui_ext {
	void Image(std::shared_ptr<moth_ui::IImage> image, int width, int height){
        auto sdlImage = std::dynamic_pointer_cast<SDLImage>(image);
		TextureRef imageTexture = sdlImage->GetTexture();
		ImGui::Image(imageTexture.get(), ImVec2(static_cast<float>(width), static_cast<float>(height)));
	}

    void Inspect(char const* name, moth_ui::IImage const* image) {
        if (image == nullptr) {
            return;
        }
        auto const sdlImage = static_cast<SDLImage const*>(image);
        auto const sdlTexture = sdlImage->GetTexture();
        auto const& sourceRect = sdlImage->GetSourceRect();
        auto const textureDimensions = static_cast<moth_ui::FloatVec2>(sdlImage->GetTextureDimensions());
        auto const uv0 = static_cast<moth_ui::FloatVec2>(sourceRect.topLeft) / textureDimensions;
        auto const uv1 = static_cast<moth_ui::FloatVec2>(sourceRect.bottomRight) / textureDimensions;

        float constexpr ImageWidth = 200;
        auto const imageDimensions = sdlImage->GetDimensions();
        auto const scale = ImageWidth / imageDimensions.x;
        auto const scaledDimensions = static_cast<moth_ui::FloatVec2>(imageDimensions) * scale;
        ImGui::Image(sdlTexture.get(), ImVec2(scaledDimensions.x, scaledDimensions.y), ImVec2(uv0.x, uv0.y), ImVec2(uv1.x, uv1.y));
    }
}
