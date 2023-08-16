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
#include <unordered_map>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <magic_enum.hpp>

#define SPDLOG_FMT_EXTERNAL
#include <spdlog/spdlog.h>

#include <range/v3/algorithm/find.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/remove_if.hpp>

#include <moth_ui/utils/rect.h>
#include <moth_ui/ui_fwd.h>

#include "utils.h"
#include "editor/imgui_ext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <vma/vk_mem_alloc.h>
