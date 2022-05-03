#pragma once

#include <tuple>
#include <type_traits>

#include <glm/glm.hpp>
#include <gl_classes/host_device_buffer.h>
#include <gl_classes/ping_pong.h>

namespace gl_containers {

    template<class... Args>
    struct GpuVectorTuplePingPong
    {
        template<class T> using buffer_type = gl_classes::PingPong<gl_classes::HostDeviceBuffer<T>>;
        using tuple_type = std::tuple<buffer_type<Args>...>;
        using tuple_size = std::tuple_size<tuple_type>;
        template<size_t Idx> using tuple_element = typename std::tuple_element<Idx, tuple_type>::type;

        void setup(glm::uint initial_capacity = 0);

        template<size_t Idx>
        tuple_element<Idx>& items() { return std::get<Idx>(m_items); }
        tuple_type& items() { return m_items; }
        gl_classes::HostDeviceBuffer<glm::uint>&  count() { return m_count; }

        static glm::uint index_count()    { return 0; }
        static glm::uint index_capacity() { return 1; }
        static glm::uint index_counter()  { return 2; }

        void debugDownloadData();
        bool enable_debug_download = false;
    protected:
        template<class TArg> void setup_items(TArg& items, glm::uint initial_capacity)
        {
            items.write().target(GL_SHADER_STORAGE_BUFFER);
            items.write().init();
            items.write().bind();
            items.write().resize(initial_capacity);

            items.read().target(GL_SHADER_STORAGE_BUFFER);
            items.read().init();
            items.read().bind();
            items.read().resize(initial_capacity);
        }
        
        template<size_t Idx, class TArg, class... TArgs, std::enable_if_t<(Idx <= tuple_size::value-2), bool> = true>
        void setup_items(glm::uint initial_capacity) 
        { 
            setup_items<TArg>(items<Idx>(), initial_capacity);
            setup_items<Idx+1, TArgs...>(initial_capacity);
        }
        template<size_t Idx, class TArg, std::enable_if_t<(Idx == tuple_size::value-1), bool> = true> 
        void setup_items(glm::uint initial_capacity)
        {
            setup_items<TArg>(items<Idx>(), initial_capacity);
        }

        template<class TArg, class... TArgs>
        void setup_items(glm::uint initial_capacity)
        {
            setup_items<0, TArg, TArgs...>(initial_capacity);
        }

        void setup_items(glm::uint initial_capacity)
        {
            setup_items<0, buffer_type<Args>...>(initial_capacity);
        }

        template<class TArg> void download_items(TArg& items)
        {
            items.read().bind();
            items.read().download();
            items.write().bind();
            items.write().download();
        }
        
        template<size_t Idx, class TArg, class... TArgs, std::enable_if_t<(Idx <= tuple_size::value-2), bool> = true>
        void download_items() 
        { 
            download_items<TArg>(items<Idx>());
            download_items<Idx+1, TArgs...>();
        }
        template<size_t Idx, class TArg, std::enable_if_t<(Idx == tuple_size::value-1), bool> = true> 
        void download_items()
        {
            download_items<TArg>(items<Idx>());
        }

        template<class TArg, class... TArgs>
        void download_items()
        {
            download_items<0, TArg, TArgs...>();
        }

        void download_items()
        {
            download_items<0, buffer_type<Args>...>();
        }

        tuple_type m_items;
        gl_classes::HostDeviceBuffer<glm::uint> m_count;

    };

} // namespace gl_containers

#include <gl_containers/gpu_vector_tuple_ping_pong.impl.h>
