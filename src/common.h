#pragma once

#include <memory>
#include <vector>
#include <variant>
#include <optional>
#include <algorithm>
#include <fstream>
#include <stack>
#include <set>
#include <filesystem>
#include <thread>

#include <SDL.h>
#include <SDL_image.h>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <magic_enum.hpp>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>

#include <moth_ui/utils/rect.h>
#include <moth_ui/ui_fwd.h>

#include "smart_sdl.h"
#include "utils.h"
