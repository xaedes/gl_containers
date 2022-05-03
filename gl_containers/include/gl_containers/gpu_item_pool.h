#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <gl_classes/host_device_buffer.h>
#include <gl_classes/ping_pong.h>

#include <gl_containers/programs.h>
#include <gl_containers/gpu_vector.h>
#include <gl_containers/gpu_vector_tuple.h>
#include <gl_containers/gpu_vector_ping_pong.h>
#include <gl_containers/gpu_vector_tuple_ping_pong.h>

namespace gl_containers { 

    template<class T>
    struct GpuItemPool
    {
        using value_type = T;

        void setup(
            const std::shared_ptr<Programs>& programs = nullptr,
            glm::uint pool_capacity = 1024UL*1024UL*5UL / sizeof(value_type), // 5m
            glm::uint erase_capacity = 1024UL*1024UL,                         // 4m
            glm::uint new_items_capacity = 1024UL*1024UL                      // 4m  
        );

        gl_classes::HostDeviceBuffer<value_type>&                       slots()                  { return m_slots; }
        gl_classes::HostDeviceBuffer<glm::uint>&                        allocated_bitset()       { return m_allocated_bitset; }
        GpuVector<glm::uint>&                                           free_slot_ids()          { return m_free_slot_ids; }
        GpuVector<glm::uint>&                                           erase_slot_ids()         { return m_erase_slot_ids; }
        GpuVectorTuple<value_type, glm::uint>&                          new_items()              { return m_new_items; }
        GpuVectorTuplePingPong<glm::uint, glm::uint>&                   created()                { return m_created; }
        
        void eraseItems();
        void createItems();

        void filterOutErasedIds(
            GpuVectorPingPong<glm::uint>& slot_ids
        );
        
        void debugDownloadData();
        bool enable_debug_download = false;

    protected:
        std::shared_ptr<Programs> m_programs;

        gl_classes::HostDeviceBuffer<value_type> m_slots;

        gl_classes::HostDeviceBuffer<glm::uint> m_allocated_bitset; // same capacity as m_slots/(8*sizeof(uint))

        GpuVector<glm::uint> m_free_slot_ids;

        // cleared after processing, by setting count to zero
        GpuVector<glm::uint> m_erase_slot_ids;

        // cleared after processing, by setting count to zero
        GpuVectorTuple<value_type, glm::uint> m_new_items;
        
        GpuVectorTuplePingPong<glm::uint, glm::uint> m_created;
    };

} // namespace gl_containers

#include <gl_containers/gpu_item_pool.impl.h>
