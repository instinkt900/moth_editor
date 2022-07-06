#pragma once

namespace imgui_ext {
	void Image(std::shared_ptr<moth_ui::IImage> image, int width, int height);
	void Inspect(char const* name, moth_ui::IImage const* value);
}
