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
    void GpuVectorPingPong<T>::resize(
        glm::uint new_size,
        bool download_count
    )
    {
        if (download_count)
        {
            count().bind();
            count().download();
        }

        auto current_size = count().buffer[index_count()];

        if (new_size == current_size) return;

        bool is_growing = new_size > current_size;
        bool fits_in_read = new_size <= items().read().size();

        if (is_growing && !fits_in_read)
        {
            m_programs->copy<T>().use();
            {
                items().read().bind();
                items().read().bufferBase(0);

                items().write().bind();
                items().write().bufferBase(1);
                items().write().resize(new_size);

                m_programs->copy<T>().offset_in.set(0);
                m_programs->copy<T>().offset_out.set(0);
            }
            m_programs->copy<T>().dispatch(
                count().buffer[index_count()]
            );
            items().toggle();
        }

        count().bind();
        count().buffer[index_count()] = new_size;
        count().upload();
    }
    
    template<class T>
    void GpuVectorPingPong<T>::grow_by(
        glm::uint growth_amount, 
        bool download_count
    )
    {
        if (growth_amount == 0) return;
        if (download_count)
        {
            count().bind();
            count().download();
        }

        resize(
            count().buffer[index_count()] + growth_amount,
            false
        );
    }

    template<class T>
    void GpuVectorPingPong<T>::append(
        gl_classes::HostDeviceBuffer<T>& items_data,
        glm::uint items_offset,
        glm::uint items_count
    )
    {
        if (items_count == 0) return;

        auto old_size = get_size(true);
        
        grow_by(items_count, false);

        // debugDownloadData();
        items().toggle();

        m_programs->copy<T>().use();
        {
            items_data.bind();
            items_data.bufferBase(0);

            items().write().bind();
            items().write().bufferBase(1);

            m_programs->copy<T>().offset_in.set(items_offset);
            m_programs->copy<T>().offset_out.set(old_size);
        }
        m_programs->copy<T>().dispatch(
            items_count
        );

        // debugDownloadData();
        items().toggle();
    }

    template<class T>
    void GpuVectorPingPong<T>::append_from_cpu(
        const T* items_data,
        glm::uint items_offset,
        glm::uint items_count
    )
    {
        if (items_count == 0) return;

        auto old_size = get_size(true);

        grow_by(items_count, false);

        items().toggle();

        items().write().bind();
        items().write().upload(
            items_data + items_offset,
            old_size,
            items_count
        );

        items().toggle();
    }

    template<class T>
    void GpuVectorPingPong<T>::debug_download_data()
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
    

    template<class T>
    glm::uint GpuVectorPingPong<T>::get_size(bool download)
    {
        if (download)
        {
            count().bind();
            count().download();
        }
        return count().buffer[index_count()];
    }

    template<class T>
    glm::uint GpuVectorPingPong<T>::get_capacity(bool download)
    {
        if (download)
        {
            count().bind();
            count().download();
        }
        return count().buffer[index_capacity()];
    }

    template<class T>
    glm::uint GpuVectorPingPong<T>::get_counter(bool download)
    {
        if (download)
        {
            count().bind();
            count().download();
        }
        return count().buffer[index_counter()];
    }
} // namespace gl_containers
