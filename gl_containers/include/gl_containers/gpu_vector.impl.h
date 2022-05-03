#pragma once

#include <gl_containers/gpu_vector.h>

namespace gl_containers {

    template<class T>
    void GpuVector<T>::setup(const std::shared_ptr<Programs>& programs, glm::uint initial_capacity)
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

        Initialize(items());
        items().bind();
        items().resize(initial_capacity);

        Initialize(count());
        count().bind();
        count().buffer.resize(3);
        count().buffer[index_count()]    = 0;
        count().buffer[index_capacity()] = initial_capacity;
        count().buffer[index_counter()]  = 0;
        count().upload();
    }

    template<class T>
    void GpuVector<T>::append(GpuVector<T>& other)
    {
        other.count().bind();
        other.count().download();

        count().bind();
        count().download();

        glm::uint offset_in = 0;
        glm::uint offset_out = count().buffer[index_count()];

        count().buffer[index_count()] += other.count().buffer[index_count()];
        count().upload();

        m_programs->copy<T>().use();
        {
            other.items().bind();
            other.items().bufferBase(0);

            items().bind();
            items().bufferBase(1);

            m_programs->copy<T>().offset_in.set(offset_in);
            m_programs->copy<T>().offset_out.set(offset_out);
        }
        m_programs->copy<T>().dispatch(
            other.count().buffer[index_count()]
        );
    }

    template<class T>
    void GpuVector<T>::clear()
    {

        count().bind();
        count().download();
        count().buffer[index_count()] = 0;
        count().upload();
    }

    template<class T>
    void GpuVector<T>::shrink_to_fit()
    {
        count().bind();
        count().download();

        if (count().buffer[index_count()] < items().size())
        {
            items().bind();
            items().resize(count().buffer[index_count()]);
        }
    }

    template<class T>
    void GpuVector<T>::debugDownloadData()
    {
        if (!enable_debug_download) return;

        auto BindAndDownload = [](auto& buf)
        {
            buf.bind();
            buf.download();
        };

        BindAndDownload(items());
        BindAndDownload(count());
    }

} // namespace gl_containers
