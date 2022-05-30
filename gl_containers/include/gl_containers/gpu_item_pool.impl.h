#pragma once

#include <gl_containers/gpu_item_pool.h>

namespace gl_containers {

    template<class T>
    void GpuItemPool<T>::setup(
        const std::shared_ptr<Programs>& programs,
        glm::uint pool_capacity,
        glm::uint erase_capacity,
        glm::uint new_items_capacity
    )
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

        constexpr auto bitset_item_width = sizeof(glm::uint)*8;

        Initialize(slots());
        slots().bind();
        slots().resize(pool_capacity);

        Initialize(allocated_bitset());
        allocated_bitset().bind();
        allocated_bitset().resize(
            (pool_capacity / bitset_item_width)
          + ((pool_capacity % bitset_item_width == 0) ? 0 : 1)
        );

        free_slot_ids().setup(m_programs, pool_capacity);
        erase_slot_ids().setup(m_programs, erase_capacity);

        m_new_items.setup(new_items_capacity);
        
        m_created.setup(0);

        clear();

    }

    template<class T>
    void GpuItemPool<T>::eraseItems()
    {
        auto& erase_from_pool = m_programs->erase_from_pool();
        erase_from_pool.use();
        {
            erase_slot_ids().items().bind();
            erase_slot_ids().items().bufferBase(0);

            // erase_slot_ids().items().download();

            allocated_bitset().bind();
            allocated_bitset().bufferBase(1);

            free_slot_ids().items().bind();
            free_slot_ids().items().bufferBase(2);

            free_slot_ids().count().bind();
            free_slot_ids().count().bufferBase(3);
            free_slot_ids().count().download();

            erase_from_pool.offset_erase_ids.set(0);
            erase_from_pool.offset_free_ids.set(0);
            erase_from_pool.index_count.set(free_slot_ids().index_count());

            erase_slot_ids().count().bind();
            erase_slot_ids().count().download();
        }
        erase_from_pool.dispatch(erase_slot_ids().count().buffer[erase_slot_ids().index_count()]);

        // free_slot_ids().count().bind();
        // free_slot_ids().count().download();

        erase_slot_ids().count().bind();
        erase_slot_ids().count().buffer[erase_slot_ids().index_count()] = 0;
        erase_slot_ids().count().upload();
    }

    template<class T>
    void GpuItemPool<T>::createItems()
    {

        new_items().count().bind();
        new_items().count().download();

        auto new_items_count = new_items().count().buffer[new_items().index_count()];

        if (new_items_count == 0) return;

        created().count().bind();
        created().count().download();

        auto created_count = created().count().buffer[created().index_count()];
        auto new_created_size = created_count + new_items_count;
        

        auto& copy_uints = m_programs->copy_uints();
        copy_uints.use();
        {

            created().items<0>().read().bind();
            created().items<0>().read().bufferBase(0);

            created().items<0>().write().bind();
            created().items<0>().write().bufferBase(1);
            created().items<0>().write().resize(new_created_size);

            copy_uints.offset_in.set(0);
            copy_uints.offset_out.set(0);
        }
        copy_uints.dispatch(created_count);

        copy_uints.use();
        {

            created().items<1>().read().bind();
            created().items<1>().read().bufferBase(0);

            created().items<1>().write().bind();
            created().items<1>().write().bufferBase(1);
            created().items<1>().write().resize(new_created_size);

            copy_uints.offset_in.set(0);
            copy_uints.offset_out.set(0);
        }
        copy_uints.dispatch(created_count);

        auto& insert_into_pool = m_programs->insert_into_pool<T>();
        insert_into_pool.use();
        {
            new_items().items<0>().bind();
            new_items().items<0>().bufferBase(0);

            new_items().items<1>().bind();
            new_items().items<1>().bufferBase(1);
            
            created().items<0>().write().bind();
            created().items<0>().write().bufferBase(2);
            
            created().items<1>().write().bind();
            created().items<1>().write().bufferBase(3);
            
            created().count().bind();
            created().count().bufferBase(4);

            slots().bind();
            slots().bufferBase(5);
            
            allocated_bitset().bind();
            allocated_bitset().bufferBase(6);
            
            free_slot_ids().items().bind();
            free_slot_ids().items().bufferBase(7);
            
            free_slot_ids().count().bind();
            free_slot_ids().count().bufferBase(8);
            free_slot_ids().count().download();
            free_slot_ids().count().buffer[free_slot_ids().index_counter()] = 0;
            free_slot_ids().count().upload();

            insert_into_pool.offset_insert_items.set(0);
            insert_into_pool.offset_insert_tickets.set(0);
            insert_into_pool.offset_created_ids.set(0);
            insert_into_pool.offset_created_tickets.set(0);
            insert_into_pool.offset_free_ids.set(0);
            insert_into_pool.index_count_created.set(0);
            insert_into_pool.index_count_free.set(free_slot_ids().index_count());
            insert_into_pool.index_capacity_free.set(free_slot_ids().index_capacity());
            insert_into_pool.index_counter_free.set(free_slot_ids().index_counter());
        }
        insert_into_pool.dispatch(new_items().count().buffer[new_items().index_count()]);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // debugDownloadData();

        created().count().bind();
        created().count().download();
        created().count().buffer[created().index_count()] = new_created_size;
        created().count().upload();

        new_items().count().bind();
        new_items().count().buffer[0] = 0;
        new_items().count().upload();

        free_slot_ids().count().bind();
        free_slot_ids().count().download();
        free_slot_ids().count().buffer[free_slot_ids().index_count()] -= free_slot_ids().count().buffer[free_slot_ids().index_counter()];
        free_slot_ids().count().buffer[free_slot_ids().index_counter()] = 0;
        free_slot_ids().count().upload();


        created().items<0>().toggle();
        created().items<1>().toggle();
    }
    
    template<class T>
    void GpuItemPool<T>::filterOutErasedIds(
        GpuVectorPingPong<glm::uint>& slot_ids
    )
    {
        auto& remove_erased_ids = m_programs->remove_erased_ids();
        remove_erased_ids.use();
        {
            slot_ids.items().read().bind();
            slot_ids.items().read().bufferBase(0);

            slot_ids.items().write().bind();
            slot_ids.items().write().bufferBase(1);

            slot_ids.items().write().resize(slot_ids.items().read().size());

            allocated_bitset().bind();
            allocated_bitset().bufferBase(2);

            slot_ids.count().bind();
            slot_ids.count().bufferBase(3);
            slot_ids.count().download();
            slot_ids.count().buffer[slot_ids.index_count()] = 0;
            slot_ids.count().upload();

            remove_erased_ids.offset_in.set(0);
            remove_erased_ids.offset_out.set(0);
            remove_erased_ids.index_count.set(0);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        remove_erased_ids.dispatch(slot_ids.items().read().size());

        glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

        slot_ids.count().bind();
        slot_ids.count().download();

        slot_ids.items().toggle();
    }

    template<class T>
    void GpuItemPool<T>::insertFromCpu(
        glm::uint* inserted_ids,
        glm::uint* inserted_tickets,
        const value_type* items,
        const glm::uint* tickets,
        glm::uint size
    )
    {
        // create previously queued new_items
        createItems();

        glm::uint created_size = created().get_size(true);
        // glm::uint created_capacity = created().get_capacity(false);
        // glm::uint created_free = created_capacity - created_size;
        // assert(created_free > 0);

        glm::uint cnt_remaining = size;

        auto read_items = items;
        auto read_tickets = tickets;

        auto write_inserted_ids = inserted_ids;
        auto write_inserted_tickets = inserted_tickets;

        while (cnt_remaining > 0)
        {
            new_items().count().bind();
            new_items().count().download();

            glm::uint new_items_size = new_items().get_size(false);
            glm::uint new_items_capacity = new_items().get_capacity(false);
            glm::uint new_items_free = new_items_capacity - new_items_size;
            assert(new_items_free > 0);

            // glm::uint free = std::min(new_items_free, created_free);

            auto cnt = std::min(new_items_free, cnt_remaining);
            
            auto& new_items_buf = new_items().items<0>();
            auto& new_tickets_buf = new_items().items<1>();

            new_items_buf.bind();
            new_items_buf.upload(
                read_items,
                new_items_size,
                cnt
            );

            new_tickets_buf.bind();
            new_tickets_buf.upload(
                read_tickets,
                new_items_size,
                cnt
            );

            new_items().count().bind();
            new_items().count().buffer[new_items().index_count()] += cnt;
            new_items().count().upload();

            cnt_remaining -= cnt;
            read_items += cnt;
            read_tickets += cnt;

            createItems();

            auto& created_ids = created().items<0>().read();
            auto& created_tickets = created().items<1>().read();

            created_ids.bind();
            created_ids.download(
                write_inserted_ids,
                created_size,
                cnt
            );

            created_tickets.bind();
            created_tickets.download(
                write_inserted_tickets,
                created_size,
                cnt
            );

            write_inserted_ids += cnt;
            write_inserted_tickets += cnt;

            created().count().bind();
            created().count().download();
            created().count().buffer[created().index_count()] = created_size;
            created().count().upload();
        }

    }

    template<class T>
    void GpuItemPool<T>::eraseItems(
        const glm::uint* erase_ids,
        glm::uint size
    )
    {
        // erase previously queued items
        eraseItems();

        glm::uint cnt_remaining = size;

        auto current = erase_ids;

        while (cnt_remaining > 0)
        {
            erase_slot_ids().count().bind();
            erase_slot_ids().count().download();

            glm::uint erase_slot_ids_size = erase_slot_ids().get_size(false);
            glm::uint erase_slot_ids_capacity = erase_slot_ids().get_capacity(false);
            glm::uint erase_slot_ids_free = erase_slot_ids_capacity - erase_slot_ids_size;
            assert(erase_slot_ids_free > 0);

            auto cnt = std::min(erase_slot_ids_free, cnt_remaining);

            erase_slot_ids().items().bind();
            erase_slot_ids().items().upload(
                current,
                erase_slot_ids_size,
                cnt
            );

            current += cnt;
            cnt_remaining -= cnt;

            erase_slot_ids().count().bind();
            erase_slot_ids().count().buffer[erase_slot_ids().index_count()] += cnt;
            erase_slot_ids().count().upload();

            eraseItems();

        }
    }

    template<class T>
    void GpuItemPool<T>::debugDownloadData()
    {
        if (!enable_debug_download) return;

        auto BindAndDownload = [](auto& buf)
        {
            buf.bind();
            buf.download();
        };

        BindAndDownload(slots());
        BindAndDownload(allocated_bitset());

        free_slot_ids().debugDownloadData();
        erase_slot_ids().debugDownloadData();
        new_items().debugDownloadData();
        created().debugDownloadData();
    }

    template<class T>
    glm::uint GpuItemPool<T>::get_size(bool download)
    {
        auto cap = m_free_slot_ids.get_capacity(download); 
        auto free = get_free(false); 
        return cap - free;
    }
    
    template<class T>
    glm::uint GpuItemPool<T>::get_free(bool download)
    {
        return m_free_slot_ids.get_size(download); 
    }
    
    template<class T>
    glm::uint GpuItemPool<T>::get_capacity(bool download) 
    {
        return m_free_slot_ids.get_capacity(download); 
    }

    template<class T>
    void GpuItemPool<T>::clear()
    {
        clear_set_free_slots();
        clear_allocated_bitset();
        clear_created();
        clear_erase_slot_ids();
        clear_new_items();
    }

    template<class T>
    void GpuItemPool<T>::clear_set_free_slots()
    {
        free_slot_ids().count().bind();
        free_slot_ids().count().buffer[free_slot_ids().index_count()] = free_slot_ids().count().buffer[free_slot_ids().index_capacity()];
        free_slot_ids().count().buffer[free_slot_ids().index_counter()] = 0;
        free_slot_ids().count().upload();

        auto& set_uint_sequence = m_programs->set_uint_sequence();
        set_uint_sequence.use();
        {
            free_slot_ids().items().bind();
            free_slot_ids().items().bufferBase(0);
        }
        set_uint_sequence.dispatch(
            free_slot_ids().count().buffer[free_slot_ids().index_count()],
            0, 1
        );

    }

    template<class T>
    void GpuItemPool<T>::clear_allocated_bitset()
    {
        auto& set_uint = m_programs->set_uint();
        set_uint.use();
        {
            allocated_bitset().bind();
            allocated_bitset().bufferBase(0);
        }
        set_uint.dispatch(
            allocated_bitset().size(),
            0
        );

    }

    template<class T>
    void GpuItemPool<T>::clear_created()
    {
        created().clear();
    }

    template<class T>
    void GpuItemPool<T>::clear_erase_slot_ids()
    {
        erase_slot_ids().clear();
    }

    template<class T>
    void GpuItemPool<T>::clear_new_items()
    {
        new_items().clear();
    }

} // namespace gl_containers
