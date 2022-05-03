#include <gl_containers/programs/remove_erased_ids_program.h>
#include <gl_containers/gpu_bitset.h>

namespace gl_containers {

    RemoveErasedIdsProgram::RemoveErasedIdsProgram()
    {

    }

    void RemoveErasedIdsProgram::setup(
        glm::uvec3 group_size
    )
    {
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &m_max_total_group_size);
        
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &m_max_group_size.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &m_max_group_size.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &m_max_group_size.z);

        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &m_max_group_count.x);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &m_max_group_count.y);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &m_max_group_count.z);

        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS " << m_max_total_group_size << "\n";
        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE.x " << m_max_group_size.x << "\n";
        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE.y " << m_max_group_size.y << "\n";
        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE.z " << m_max_group_size.z << "\n";
        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT.x " << m_max_group_count.x << "\n";
        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT.y " << m_max_group_count.y << "\n";
        // std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT.z " << m_max_group_count.z << "\n";
        if (
            (group_size.x > m_max_group_size.x)
         || (group_size.y > m_max_group_size.y)
         || (group_size.z > m_max_group_size.z)
        )
        {
            throw std::runtime_error(
                "Group size ("
                 + std::to_string(group_size.x) + ", "
                 + std::to_string(group_size.y) + ", "
                 + std::to_string(group_size.z)
                 + ") exceeds maximum allowed size of ("
                 + std::to_string(m_max_group_size.x) + ", "
                 + std::to_string(m_max_group_size.y) + ", "
                 + std::to_string(m_max_group_size.z) + ")"
            );
        }
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
        offset_in.init(getGlProgram(), "offset_in", 0);
        offset_out.init(getGlProgram(), "offset_out", 0);
        offset_next_free.init(getGlProgram(), "offset_next_free", 0);
        index_count.init(getGlProgram(), "index_count", 0);
        gl_classes::checkGLError();

    }

    void RemoveErasedIdsProgram::dispatch(glm::uint num_items)
    {
        this->num_items.set(num_items);
        ComputeProgram::dispatch(num_items, 1, 1, m_group_size.x, m_group_size.y, m_group_size.z);
    }

    std::string RemoveErasedIdsProgram::code_compute()
    {
        return (
            R"(
            #version 440
            #define GROUPSIZE_X ##GROUPSIZE_X##
            #define GROUPSIZE_Y ##GROUPSIZE_Y##
            #define GROUPSIZE_Z ##GROUPSIZE_Z##
            #define GROUPSIZE (GROUPSIZE_X*GROUPSIZE_Y*GROUPSIZE_Z)
            layout(local_size_x=GROUPSIZE_X, local_size_y=GROUPSIZE_Y, local_size_z=GROUPSIZE_Z) in;

            layout(std430, binding = 0) buffer buf0 { uint in_ids[]; };
            layout(std430, binding = 1) buffer buf1 { uint out_ids[]; };
            layout(std430, binding = 2) buffer buf2 { uint allocated_bitset[]; };
            layout(std430, binding = 3) buffer buf3 { uint count[]; };

            uniform uint num_items;
            uniform uint offset_in = 0;
            uniform uint offset_out = 0;
            uniform uint offset_next_free = 0;
            uniform uint index_count = 0;

            )" + code_bitset("allocated_bitset") + R"(

            void main() {
                uint workgroup_idx = 
                    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y +
                    gl_WorkGroupID.y * gl_NumWorkGroups.x +
                    gl_WorkGroupID.x;
                uint global_idx = gl_LocalInvocationIndex + workgroup_idx * GROUPSIZE;
                if (global_idx >= num_items) return;

                uint slot_id = in_ids[global_idx + offset_in];

                if (bitset_get(slot_id) == true)
                {
                    // occupied slot
                    uint out_idx = atomicAdd(count[index_count], 1);
                    out_ids[offset_out + out_idx] = slot_id;
                }
            }
            )"
        );
    }



} // namespace gl_containers 
