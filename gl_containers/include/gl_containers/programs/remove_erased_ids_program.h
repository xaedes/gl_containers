#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <gl_classes/program.h>
#include <gl_classes/program_uniform.h>
#include <gl_classes/compute_program.h>
#include <gl_classes/shader.h>

namespace gl_containers {

    class RemoveErasedIdsProgram : public gl_classes::ComputeProgram
    {
    public:
        using Program = gl_classes::Program;
        using ComputeProgram = gl_classes::ComputeProgram;
        using Shader = gl_classes::Shader;
        template<class T> using ProgramUniform = gl_classes::ProgramUniform<T>;

        RemoveErasedIdsProgram();
        ~RemoveErasedIdsProgram(){}

        void setup(
            glm::uvec3 group_size = glm::uvec3(1024,1,1)
        );

        void dispatch(glm::uint num_items);

        static std::string code_compute();

        ProgramUniform<glm::uint> num_items;
        ProgramUniform<glm::uint> offset_in;
        ProgramUniform<glm::uint> offset_out;
        ProgramUniform<glm::uint> offset_next_free;
        ProgramUniform<glm::uint> index_count;

        glm::uvec3 group_size() const { return m_group_size; }
    protected:
        glm::uvec3 m_group_size;
        glm::ivec3 m_max_group_size;
        glm::ivec3 m_max_group_count;
        int m_max_total_group_size;
    };

}  // namespace gl_containers

