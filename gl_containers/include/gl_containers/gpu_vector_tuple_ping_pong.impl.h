#pragma once

#include <gl_containers/gpu_vector_tuple_ping_pong.h>

namespace gl_containers {

    template<class... Args>
    void GpuVectorTuplePingPong<Args...>::setup(glm::uint initial_capacity)
    {
        auto Initialize = [](auto& buf)
        {
            buf.target(GL_SHADER_STORAGE_BUFFER);
            buf.init();
        };

        setup_items(initial_capacity);

        Initialize(count());
        count().bind();
        count().buffer.resize(3);
        count().buffer[index_count()]    = 0;
        count().buffer[index_capacity()] = initial_capacity;
        count().buffer[index_counter()]  = 0;
        count().upload();
    }

    template<class... Args>
    void GpuVectorTuplePingPong<Args...>::debugDownloadData()
    {
        if (!enable_debug_download) return;

        auto BindAndDownload = [](auto& buf)
        {
            buf.bind();
            buf.download();
        };

        download_items();
        BindAndDownload(count());
    }

    template<class... Args>
    glm::uint GpuVectorTuplePingPong<Args...>::get_size(bool download = true)
    {
        if (download)
        {
            count().bind();
            count().download();
        }
        return count().buffer[index_count()];
    }

    template<class... Args>
    glm::uint GpuVectorTuplePingPong<Args...>::get_capacity(bool download = true)
    {
        if (download)
        {
            count().bind();
            count().download();
        }
        return count().buffer[index_capacity()];
    }

    template<class... Args>
    glm::uint GpuVectorTuplePingPong<Args...>::get_counter(bool download = true)
    {
        if (download)
        {
            count().bind();
            count().download();
        }
        return count().buffer[index_counter()];
    }

    template<class... Args>
    void GpuVectorTuplePingPong<Args...>::clear()
    {
        count().bind();
        count().download();
        count().buffer[index_count()] = 0;
        count().upload();
    }

} // namespace gl_containers
