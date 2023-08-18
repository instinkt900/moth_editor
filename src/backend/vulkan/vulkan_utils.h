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
    inline uint32_t CalcHash(T const& source) {
        return murmurhash(reinterpret_cast<char const*>(&source), sizeof(T), HashSeed);
    }

    inline uint32_t CalcHash(std::string const& source) {
        return murmurhash(source.c_str(), static_cast<uint32_t>(source.length()), HashSeed);
    }

    inline uint32_t CalcHash(void const* source, size_t sourceSize) {
        return murmurhash(static_cast<char const*>(source), static_cast<uint32_t>(sourceSize), HashSeed);
    }

    inline std::vector<std::string_view> split_str(std::string const& str, char const delim = ' ') {
        std::vector<std::string_view> result;

        int leftIndex = 0;
        int rightIndex = -1;

        for (int i = 0; i < static_cast<int>(str.size()); ++i) {
            if (str[i] == delim) {
                leftIndex = rightIndex + 1;
                rightIndex = i;
                std::string_view token(str.data() + leftIndex, rightIndex - leftIndex);
                result.push_back(token);
            }
        }

        std::string_view finalToken(str.data() + rightIndex + 1, str.size() - rightIndex - 1);
        result.push_back(finalToken);
        return result;
    }
}
