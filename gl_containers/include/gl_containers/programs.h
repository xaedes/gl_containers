#pragma once

#include <typeindex>
#include <unordered_map>

#include <gl_classes/compute_programs/set_sequence_program.h>
#include <gl_classes/compute_programs/set_values_program.h>
#include <gl_classes/compute_programs/copy_program.h>

#include <gl_containers/programs/erase_from_pool_program.h>
#include <gl_containers/programs/remove_erased_ids_program.h>
#include <gl_containers/programs/insert_into_pool_program.h>

namespace gl_containers {

    struct Programs
    {
        void setup();

        EraseFromPoolProgram&                                        erase_from_pool()                      { return m_erase_from_pool;                      }
        RemoveErasedIdsProgram&                                      remove_erased_ids()                    { return m_remove_erased_ids;                    }
        gl_classes::compute_programs::SetSequenceProgram<glm::uint>& set_uint_sequence()                    { return m_set_uint_sequence;                    }
        gl_classes::compute_programs::SetValuesProgram<glm::uint>&   set_uint()                             { return m_set_uint;                             }
        gl_classes::compute_programs::CopyProgram&                   copy_uints()                           { return m_copy_uints;                           }

        template<class T> 
        struct InsertIntoPool
        {
            static InsertIntoPoolProgram& get(Programs& programs)
            {
                std::type_index typeidx = typeid(int);
                if (programs.m_insert_into_pool.count(typeidx) == 0)
                {
                    // dynamically create program
                    programs.m_insert_into_pool[typeidx].setup(
                        // type T must provide std::string returning functions code_definition() and code_name()
                        T::code_definition(), 
                        T::code_name()
                    );
                }
                return programs.m_insert_into_pool[typeidx];
            }
        };
        
        // Example how to extend the template for specific types, to avoid the use of unordered_map:
        // 
        // template<>
        // struct InsertIntoPool<MyType>
        // {
        //     static InsertIntoPoolProgram& get(Programs& programs)
        //     {
        //         return programs.insert_mytype_into_pool();
        //     }
        // };
        // InsertIntoPoolProgram& insert_mytype_into_pool() { return m_insert_mytype_into_pool; }
        // protected:
        // InsertIntoPoolProgram m_insert_mytype_into_pool;

        template<class T>
        InsertIntoPoolProgram& insert_into_pool() { return InsertIntoPool<T>::get(*this); }

        template<class T> 
        struct Copy
        {};
        template<>
        struct Copy<glm::uint>
        {
            static gl_classes::compute_programs::CopyProgram& get(Programs& programs)
            {
                return programs.copy_uints();
            }
        };
        template<class T>
        gl_classes::compute_programs::CopyProgram& copy() { return Copy<T>::get(*this); }

    protected:
        std::unordered_map<std::type_index, InsertIntoPoolProgram>  m_insert_into_pool;
        EraseFromPoolProgram                                        m_erase_from_pool;
        RemoveErasedIdsProgram                                      m_remove_erased_ids;
        gl_classes::compute_programs::SetSequenceProgram<glm::uint> m_set_uint_sequence;
        gl_classes::compute_programs::SetValuesProgram<glm::uint>   m_set_uint;
        gl_classes::compute_programs::CopyProgram                   m_copy_uints;

    };

} // end namespace gl_containers
