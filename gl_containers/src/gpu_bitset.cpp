#pragma once

#include <string>

namespace gl_containers {

    std::string code_bitset(const std::string& bitset_buffer_name, const std::string& function_prefix)
    {
        return (
        R"(
        uvec2 )" + function_prefix + R"(_idx(uint idx)
        {
            uvec2 bs_idx;
            bs_idx.x = bitfieldExtract(idx, 5, 32-5);
            bs_idx.y = bitfieldExtract(idx, 0, 5);
            return bs_idx;
        }

        bool )" + function_prefix + R"(_get(uint idx)
        {
            uvec2 bs_idx = )" + function_prefix + R"(_idx(idx);
            uint result = ()" + bitset_buffer_name + R"([bs_idx.x] >> bs_idx.y) & uint(1);
            return result == uint(1);
        }

        void )" + function_prefix + R"(_set_true(uint idx)
        {
            uvec2 bs_idx = )" + function_prefix + R"(_idx(idx);
            atomicOr(
                )" + bitset_buffer_name + R"([bs_idx.x],
                (uint(1) << bs_idx.y)
            );
        }

        void )" + function_prefix + R"(_set_false(uint idx)
        {
            uvec2 bs_idx = )" + function_prefix + R"(_idx(idx);
            atomicAnd(
                )" + bitset_buffer_name + R"([bs_idx.x],
                ~(uint(1) << bs_idx.y)
            );
        }
        void )" + function_prefix + R"(_set(uint idx, bool value)
        {
            if (value) 
                )" + function_prefix + R"(_set_true(idx);
            else
                )" + function_prefix + R"(_set_false(idx);
        }
        )"
        );
    }

} // namespace gl_containers
