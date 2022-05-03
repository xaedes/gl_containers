#include <gl_containers/programs.h>

namespace gl_containers {

    void Programs::setup()
    {
        m_erase_from_pool.setup();
        m_remove_erased_ids.setup();
        m_set_uint.setup("uint");
        m_set_uint_sequence.setup("uint");
        m_copy_uints.setup("uint");
    }

} // namespace gl_containers
