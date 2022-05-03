#include <gl_containers/programs/insert_into_pool_program.h>
#include <gl_containers/gpu_bitset.h>

namespace gl_containers {

    InsertIntoPoolProgram::InsertIntoPoolProgram()
    {

    }

    void InsertIntoPoolProgram::setup(
        const std::string& code_item_type_definition,
        const std::string& code_item_type_name,
        glm::uvec3 group_size
    )
    {
        m_code_item_type_definition = code_item_type_definition;
        m_code_item_type_name = code_item_type_name;
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
        offset_insert_items.init(getGlProgram(), "offset_insert_items", 0);
        offset_insert_tickets.init(getGlProgram(), "offset_insert_tickets", 0);
        offset_created_ids.init(getGlProgram(), "offset_created_ids", 0);
        offset_created_tickets.init(getGlProgram(), "offset_created_tickets", 0);
        offset_free_ids.init(getGlProgram(), "offset_free_ids", 0);
        index_count_created.init(getGlProgram(), "index_count_created", 0);
        index_count_free.init(getGlProgram(), "index_count_free", 0);
        index_capacity_free.init(getGlProgram(), "index_capacity_free", 1);
        index_counter_free.init(getGlProgram(), "index_counter_free", 2);
        gl_classes::checkGLError();
    }           

    void InsertIntoPoolProgram::dispatch(glm::uint num_items)
    {
        this->num_items.set(num_items);
        ComputeProgram::dispatch(num_items, 1, 1, m_group_size.x, m_group_size.y, m_group_size.z);
    }

    std::string InsertIntoPoolProgram::code_compute() const
    {
        return (
            R"(
            #version 440
            #define GROUPSIZE_X ##GROUPSIZE_X##
            #define GROUPSIZE_Y ##GROUPSIZE_Y##
            #define GROUPSIZE_Z ##GROUPSIZE_Z##
            #define GROUPSIZE (GROUPSIZE_X*GROUPSIZE_Y*GROUPSIZE_Z)
            layout(local_size_x=GROUPSIZE_X, local_size_y=GROUPSIZE_Y, local_size_z=GROUPSIZE_Z) in;

            )" + m_code_item_type_definition + R"(

            layout(std430, binding = 0) buffer buf0 { )" + m_code_item_type_name + R"( insert_items[]; };
            layout(std430, binding = 1) buffer buf1 { uint insert_tickets[]; }; 
            layout(std430, binding = 2) buffer buf2 { uint created_ids[]; }; 
            layout(std430, binding = 3) buffer buf3 { uint created_tickets[]; }; 
            layout(std430, binding = 4) buffer buf4 { uint count_created[]; }; 
            layout(std430, binding = 5) buffer buf5 { )" + m_code_item_type_name + R"( item_pool[]; };
            layout(std430, binding = 6) buffer buf6 { uint allocated_bitset[]; };
            layout(std430, binding = 7) buffer buf7 { uint free_ids[]; };
            layout(std430, binding = 8) buffer buf8 { uint count_free[]; };

            uniform uint num_items;
            uniform uint offset_insert_items = 0;
            uniform uint offset_insert_tickets = 0;
            uniform uint offset_created_ids = 0;
            uniform uint offset_created_tickets = 0;
            uniform uint offset_free_ids = 0;
            uniform uint index_count_created = 0;
            uniform uint index_count_free = 0;
            uniform uint index_capacity_free = 1;
            uniform uint index_counter_free = 2;

            )" + code_bitset("allocated_bitset") + R"(

            void main() {
                uint workgroup_idx = 
                    gl_WorkGroupID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y +
                    gl_WorkGroupID.y * gl_NumWorkGroups.x +
                    gl_WorkGroupID.x;
                uint global_idx = gl_LocalInvocationIndex + workgroup_idx * GROUPSIZE;
                if (global_idx >= num_items) return;

                // Attention! Make sure before dispatch, that:
                // count_free[index_count_free] at first invocation must be greater than or equal num_items

                uint reverse_idx_free = atomicAdd(
                    count_free[index_counter_free],
                    uint(1)
                );

                uint idx_free = count_free[index_count_free] - uint(1) - reverse_idx_free;

                // uint idx_free = atomicAdd(
                //     count_free[index_count_free],
                //     uint(0xffffffff) // two-complement of 1, i.e. subtract 1
                // ) - 1;

                uint new_id = free_ids[offset_free_ids + idx_free];

                uint ticket = insert_tickets[offset_insert_tickets + global_idx];

                item_pool[new_id] = insert_items[offset_insert_items + global_idx];

                bitset_set_true(new_id);

                uint created_id = count_created[index_count_created] + global_idx;

                created_ids[offset_created_ids + created_id] = new_id;
                created_tickets[offset_created_tickets + created_id] = ticket;
            }
            )"
        );
    }



} // namespace gl_containers 
