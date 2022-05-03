#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <gl_classes/host_device_buffer.h>
#include <gl_classes/ping_pong.h>
#include <gl_containers/programs.h>

namespace gl_containers {

    template<class T>
    struct GpuVectorPingPong
    {
        using value_type = T;

        void setup(const std::shared_ptr<Programs>& programs, glm::uint initial_capacity = 0);

        gl_classes::PingPong<gl_classes::HostDeviceBuffer<value_type>>& items() { return m_items; }
        gl_classes::HostDeviceBuffer<glm::uint>&  count() { return m_count; }

        static glm::uint index_count()    { return 0; }
        static glm::uint index_capacity() { return 1; }
        static glm::uint index_counter()  { return 2; }

        void append(
            gl_classes::HostDeviceBuffer<T>& append,
            glm::uint offset,
            glm::uint count
        );
        void clear();
        void shrink_to_fit();
        void debugDownloadData();
        bool enable_debug_download = false;

    protected:
        std::shared_ptr<Programs> m_programs;
        
        gl_classes::PingPong<gl_classes::HostDeviceBuffer<value_type>> m_items;
        gl_classes::HostDeviceBuffer<glm::uint> m_count;

    };

} // namespace gl_containers

#include <gl_containers/gpu_vector_ping_pong.impl.h>