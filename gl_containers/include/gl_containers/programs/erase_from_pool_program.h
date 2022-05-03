#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <gl_classes/program.h>
#include <gl_classes/program_uniform.h>
#include <gl_classes/compute_program.h>
#include <gl_classes/shader.h>

namespace gl_containers {

    class EraseFromPoolProgram : public gl_classes::ComputeProgram
    {
    public:
        using Program = gl_classes::Program;
        using ComputeProgram = gl_classes::ComputeProgram;
        using Shader = gl_classes::Shader;
        template<class T> using ProgramUniform = gl_classes::ProgramUniform<T>;

        EraseFromPoolProgram();
        ~EraseFromPoolProgram(){}

        void setup(
            glm::uvec3 group_size = glm::uvec3(1024,1,1)
        );

        void dispatch(glm::uint num_items);

        static std::string code_compute();

        ProgramUniform<glm::uint> num_items;
        ProgramUniform<glm::uint> offset_erase_ids;
        ProgramUniform<glm::uint> offset_free_ids;
        ProgramUniform<glm::uint> index_count;

        glm::uvec3 group_size() const { return m_group_size; }
    protected:
        glm::uvec3 m_group_size;
    };

}  // namespace gl_containers

