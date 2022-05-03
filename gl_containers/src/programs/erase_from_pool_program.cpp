#include <gl_containers/programs/erase_from_pool_program.h>
#include <gl_containers/gpu_bitset.h>

namespace gl_containers {

    EraseFromPoolProgram::EraseFromPoolProgram()
    {

    }

    void EraseFromPoolProgram::setup(
        glm::uvec3 group_size
    )
    {
        m_group_size = group_size;
        m_shaders = {Shader(Shader::ShaderType::Compute, code_compute())};
        m_shaders[0].setup({
            {"##GROUPSIZE_X##", std::to_string(m_group_size.x)},
            {"##GROUPSIZE_Y##", std::to_string(m_group_size.y)},
            {"##GROUPSIZE_Z##", std::to_string(m_group_size.z)}
        });
        // std::cout << m_shaders[0].getCode() << std::endl;
        Program::setup();
        num_items.init(getGlProgram(), "num_items");
        offset_erase_ids.init(getGlProgram(), "offset_erase_ids", 0);
        offset_free_ids.init(getGlProgram(), "offset_free_ids", 0);
        index_count.init(getGlProgram(), "index_count", 0);
        gl_classes::checkGLError();
    }           

    void EraseFromPoolProgram::dispatch(glm::uint num_items)
    {
        this->num_items.set(num_items);
        ComputeProgram::dispatch(num_items, 1, 1, m_group_size.x, m_group_size.y, m_group_size.z);
    }

    std::string EraseFromPoolProgram::code_compute()
    {
        return (
            R"(
            #version 440
            #define GROUPSIZE_X ##GROUPSIZE_X##
            #define GROUPSIZE_Y ##GROUPSIZE_Y##
            #define GROUPSIZE_Z ##GROUPSIZE_Z##
            #define GROUPSIZE (GROUPSIZE_X*GROUPSIZE_Y*GROUPSIZE_Z)
            layout(local_size_x=GROUPSIZE_X, local_size_y=GROUPSIZE_Y, local_size_z=GROUPSIZE_Z) in;

            layout(std430, binding = 0) buffer buf0 { uint erase_ids[]; }; // must be unique!
            layout(std430, binding = 1) buffer buf1 { uint allocated_bitset[]; }; 
            layout(std430, binding = 2) buffer buf2 { uint free_ids[]; };
            layout(std430, binding = 3) buffer buf3 { uint count_free[]; };

            uniform uint num_items;
            uniform uint offset_erase_ids = 0;
            uniform uint offset_free_ids = 0;
            uniform uint index_count = 0;

            )" + code_bitset("allocated_bitset") + R"(

            void main() {
                uint workgroup_idx = 
                    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y +
                    gl_WorkGroupID.y * gl_NumWorkGroups.x +
                    gl_WorkGroupID.x;
                uint global_idx = gl_LocalInvocationIndex + workgroup_idx * GROUPSIZE;
                if (global_idx >= num_items) return;

                uint erase_id = erase_ids[offset_erase_ids + global_idx];

                bitset_set_false(erase_id);

                uint idx_free = atomicAdd(count_free[index_count], 1);
                free_ids[offset_free_ids + idx_free] = erase_id;
            }
            )"
        );
    }



} // namespace gl_containers 
