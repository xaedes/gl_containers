#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <gl_classes/program.h>
#include <gl_classes/program_uniform.h>
#include <gl_classes/compute_program.h>
#include <gl_classes/shader.h>

namespace gl_containers {

    class InsertIntoPoolProgram : public gl_classes::ComputeProgram
    {
    public:
        using Program = gl_classes::Program;
        using ComputeProgram = gl_classes::ComputeProgram;
        using Shader = gl_classes::Shader;
        template<class T> using ProgramUniform = gl_classes::ProgramUniform<T>;

        InsertIntoPoolProgram();
        ~InsertIntoPoolProgram(){}

        void setup(
            const std::string& code_item_type_definition,
            const std::string& code_item_type_name,
            glm::uvec3 group_size = glm::uvec3(1024,1,1)
        );

        void dispatch(glm::uint num_items);

        std::string code_compute() const;

        ProgramUniform<glm::uint> num_items;
        ProgramUniform<glm::uint> offset_insert_items;
        ProgramUniform<glm::uint> offset_insert_tickets;
        ProgramUniform<glm::uint> offset_created_ids;
        ProgramUniform<glm::uint> offset_created_tickets;
        ProgramUniform<glm::uint> offset_free_ids;
        ProgramUniform<glm::uint> index_count_created;
        ProgramUniform<glm::uint> index_count_free;
        ProgramUniform<glm::uint> index_capacity_free;
        ProgramUniform<glm::uint> index_counter_free;

        glm::uvec3 group_size() const { return m_group_size; }
    protected:
        glm::uvec3 m_group_size;
        std::string m_code_item_type_definition;
        std::string m_code_item_type_name;
    };

}  // namespace gl_containers

