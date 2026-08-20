#pragma once

#include "moth_ui/asset_id.h"

#include <filesystem>
#include <system_error>

// Turns a picked image path into the identity a layout stores.
//
// The asset list and the file dialog both hand over an absolute path, and a layout has
// to keep something that still resolves on another machine. moth_ui used to make this
// relative when it saved and absolute again when it loaded. It no longer touches the
// value, so the editor normalizes it here instead. See moth_ui::AssetId.
//
// The layout file's own directory is the base, which is the convention Layout::Save used
// to apply. An unsaved layout has no directory yet, so the path stays as it is and the
// next save leaves it alone. That is visible in the properties panel rather than silent.
inline moth_ui::AssetId MakeImageId(std::filesystem::path const& picked,
                                    std::filesystem::path const& layoutPath) {
    if (layoutPath.empty()) {
        return moth_ui::AssetId{ picked };
    }
    std::error_code error;
    auto const relative = std::filesystem::relative(picked, layoutPath.parent_path(), error);
    if (error || relative.empty()) {
        return moth_ui::AssetId{ picked };
    }
    return moth_ui::AssetId{ relative };
}
