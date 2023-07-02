#pragma once

#include "murmurhash.h"

#define CHECK_VK_RESULT(expr)                                                                                   \
    {                                                                                                           \
        VkResult result_ = expr;                                                                                \
        if (result_ != VK_SUCCESS) {                                                                            \
            spdlog::error("File: {} Line: {} {} = {}", __FILE__, __LINE__, #expr, static_cast<int>(result_));   \
            abort();                                                                                            \
        }                                                                                                       \
    }

namespace backend::vulkan {
    static constexpr uint32_t HashSeed = 0xc00db11c;

    template<typename T>
    static uint32_t CalcHash(T const& source) {
        return murmurhash(reinterpret_cast<char const*>(&source), sizeof(T), HashSeed);
    }

    static uint32_t CalcHash(std::string const& source) {
        return murmurhash(source.c_str(), static_cast<uint32_t>(source.length()), HashSeed);
    }

    static uint32_t CalcHash(void const* source, size_t sourceSize) {
        return murmurhash(static_cast<char const*>(source), static_cast<uint32_t>(sourceSize), HashSeed);
    }
}
