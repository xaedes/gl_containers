#pragma once

#include <gl_containers/gpu_vector_ping_pong.h>

namespace gl_containers {

    template<class T>
    void GpuVectorPingPong<T>::setup(const std::shared_ptr<Programs>& programs, glm::uint initial_capacity)
    {
        if (programs.get() == nullptr)
        {
            m_programs = std::make_shared<Programs>();
            m_programs->setup();
        }
        else
        {
            m_programs = programs;
        }

        auto Initialize = [](auto& buf)
        {
            buf.target(GL_SHADER_STORAGE_BUFFER);
            buf.init();
        };

        Initialize(items().read());
        items().read().bind();
        items().read().resize(initial_capacity);

        Initialize(items().write());
        items().write().bind();
        items().write().resize(initial_capacity);

        Initialize(count());
        count().bind();
        count().buffer.resize(3);
        count().buffer[index_count()]    = 0;
        count().buffer[index_capacity()] = initial_capacity;
        count().buffer[index_counter()]  = 0;
        count().upload();
    }

    template<class T>
    void GpuVectorPingPong<T>::clear()
    {
        count().bind();
        count().download();
        count().buffer[index_count()]    = 0;
        count().upload();
    }

    template<class T>
    void GpuVectorPingPong<T>::shrink_to_fit()
    {
        count().bind();
        count().download();

        items().toggle();
        if (count().buffer[index_count()] < items().write().size())
        {
            items().write().bind();
            items().write().resize(count().buffer[index_count()]);
        }
        items().toggle();
    }

    template<class T>
    void GpuVectorPingPong<T>::append(
        gl_classes::HostDeviceBuffer<T>& append,
        glm::uint append_offset,
        glm::uint append_count
    )
    {
        count().bind();
        count().download();

        // debugDownloadData();

        m_programs->copy<T>().use();
        {
            items().read().bind();
            items().read().bufferBase(0);

            items().write().bind();
            items().write().bufferBase(1);
            items().write().resize(count().buffer[index_count()] + append_count);

            m_programs->copy<T>().offset_in.set(0);
            m_programs->copy<T>().offset_out.set(0);
        }
        m_programs->copy<T>().dispatch(
            count().buffer[index_count()]
        );

        glm::uint offset_out = count().buffer[index_count()];

        count().buffer[index_count()] += append_count;
        count().bind();
        count().upload();

        // debugDownloadData();

        m_programs->copy<T>().use();
        {
            append.bind();
            append.bufferBase(0);

            items().write().bind();
            items().write().bufferBase(1);

            m_programs->copy<T>().offset_in.set(append_offset);
            m_programs->copy<T>().offset_out.set(offset_out);
        }
        m_programs->copy<T>().dispatch(
            append_count
        );

        // debugDownloadData();
        items().toggle();
    }

    template<class T>
    void GpuVectorPingPong<T>::debugDownloadData()
    {
        if (!enable_debug_download) return;

        auto BindAndDownload = [](auto& buf)
        {
            buf.bind();
            buf.download();
        };

        BindAndDownload(items().read());
        BindAndDownload(items().write());
        BindAndDownload(count());
    }

} // namespace gl_containers
