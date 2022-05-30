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
            glm::uint pool_capacity = 1024UL*1024UL*5UL / sizeof(value_type), // 5mitems
            glm::uint erase_capacity = 1024UL*1024UL,                         // 4mb
            glm::uint new_items_capacity = 1024UL*1024UL                      // 4mb
        );

        gl_classes::HostDeviceBuffer<value_type>&                       slots()                  { return m_slots; }
        gl_classes::HostDeviceBuffer<glm::uint>&                        allocated_bitset()       { return m_allocated_bitset; }
        GpuVector<glm::uint>&                                           free_slot_ids()          { return m_free_slot_ids; }
        GpuVector<glm::uint>&                                           erase_slot_ids()         { return m_erase_slot_ids; }
        GpuVectorTuple<value_type, glm::uint>&                          new_items()              { return m_new_items; }
        GpuVectorTuplePingPong<glm::uint, glm::uint>&                   created()                { return m_created; }
        
        void clear();
        void eraseItems();
        void createItems();

        void insertFromCpu(
            glm::uint* inserted_ids,
            glm::uint* inserted_tickets,
            const value_type* items,
            const glm::uint* tickets,
            glm::uint size
        );

        void eraseItems(
            const glm::uint* erase_ids,
            glm::uint size
        );

        
        void filterOutErasedIds(
            GpuVectorPingPong<glm::uint>& slot_ids
        );
        
        void debugDownloadData();
        bool enable_debug_download = false;

        glm::uint get_size(bool download = true);
        glm::uint get_free(bool download = true);
        glm::uint get_capacity(bool download = true);

    protected:
        void clear_set_free_slots();
        void clear_allocated_bitset();
        void clear_created();
        void clear_erase_slot_ids();
        void clear_new_items();

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
