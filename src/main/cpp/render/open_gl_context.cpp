/*!
 * \author ddubois 
 * \date 26-Feb-18.
 */

#include <glad/glad.h>
#include "open_gl_context.h"

namespace nova {
    void open_gl_context::set_default_state() {
        set_blending_enabled(false);
        set_culling_enabled(true);
        set_culling_mode(GL_BACK);
        set_depth_write_enabled(true);
        set_depth_test_enabled(true);
        set_stencil_test_enabled(false);
        set_stencil_write_enabled(false);
        set_color_write_enabled(true);
        set_alpha_to_coverage_enabled(false);
        set_alpha_write_enabled(false);
    }
}
