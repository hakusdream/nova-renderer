/*!
 * \author ddubois 
 * \date 26-Feb-18.
 */

#include <glad/glad.h>
#include <easylogging++.h>
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

    void open_gl_context::set_blending_enabled(bool enabled) {
        next_state.is_blending_enabled = enabled;
    }

    void open_gl_context::set_culling_enabled(bool enabled) {
        next_state.is_culling_eanbled = enabled;
    }

    void open_gl_context::set_culling_mode(GLenum culling_mode) {
        next_state.culling_mode = culling_mode;
    }

    void open_gl_context::set_depth_write_enabled(bool enabled) {
        next_state.is_depth_write_enabled = enabled;
    }

    void open_gl_context::set_depth_test_enabled(bool enabled) {
        next_state.is_depth_test_enabled = enabled;
    }

    void open_gl_context::set_stencil_write_enabled(bool enabled) {
        next_state.is_stencil_write_enabled = enabled;
    }

    void open_gl_context::set_stencil_test_enabled(bool enabled) {
        next_state.is_stencil_test_enabled = enabled;
    }

    void open_gl_context::set_stencil_op_separate(int face, int fail_op, int pass_op, int depth_fail_op) {
        if(face == GL_FRONT || face == GL_FRONT_AND_BACK) {
            next_state.frontface_state.fall_op = static_cast<GLenum>(fail_op);
            next_state.frontface_state.pass_op = static_cast<GLenum>(pass_op);
            next_state.frontface_state.depth_fail_op = static_cast<GLenum>(depth_fail_op);

        }

        if(face == GL_BACK || face == GL_FRONT_AND_BACK) {
            next_state.backface_state.fall_op = static_cast<GLenum>(fail_op);
            next_state.backface_state.pass_op = static_cast<GLenum>(pass_op);
            next_state.backface_state.depth_fail_op = static_cast<GLenum>(depth_fail_op);
        }
    }

    void open_gl_context::set_stencil_func_separate(int face, int compare_op, int reference, int mask) {
        if(face == GL_FRONT || face == GL_FRONT_AND_BACK) {
            next_state.frontface_state.compare_op = compare_op;
            next_state.frontface_state.reference = reference;
            next_state.frontface_state.mask = mask;

        }

        if(face == GL_BACK || face == GL_FRONT_AND_BACK) {
            next_state.backface_state.compare_op = compare_op;
            next_state.backface_state.reference = reference;
            next_state.backface_state.mask = mask;
        }
    }

    void open_gl_context::set_color_write_enabled(bool enabled) {
        next_state.is_color_write_enabled = enabled;
    }

    void open_gl_context::set_alpha_write_enabled(bool enabled) {
        next_state.is_alpha_write_enabled = enabled;
    }

    void open_gl_context::set_alpha_to_coverage_enabled(bool enabled) {
        next_state.is_alpha_to_coverage_enabled = enabled;
    }

    void open_gl_context::bind_texture(const texture2D &texture, const uint32_t texture_unit) {
        next_state.bound_textures[texture_unit] = texture.get_gl_name();
    }

    void open_gl_context::commit() {
        if(next_state.is_blending_enabled != current_state.is_blending_enabled) {
            if(next_state.is_blending_enabled) {
                glEnable(GL_BLEND);
            } else {
                glDisable(GL_BLEND);
            }
        }

        if(next_state.src_color != current_state.src_color || next_state.dst_color != current_state.dst_color
           || next_state.src_alpha != current_state.src_alpha || next_state.dst_alpha != current_state.dst_alpha) {
            glBlendFuncSeparate(next_state.src_color, next_state.dst_color, next_state.src_alpha, next_state.dst_alpha);
        }

        if(next_state.is_culling_eanbled != current_state.is_culling_eanbled) {
            if(next_state.is_culling_eanbled) {
                glEnable(GL_CULL_FACE);
            } else {
                glDisable(GL_CULL_FACE);
            }
        }

        if(next_state.culling_mode != current_state.culling_mode) {
            glCullFace(next_state.culling_mode);
        }

        if(next_state.is_depth_write_enabled != current_state.is_depth_write_enabled) {
            glDepthMask(next_state.is_depth_write_enabled ? GL_TRUE : GL_FALSE);
        }

        if(next_state.is_depth_test_enabled != current_state.is_depth_test_enabled) {
            if(next_state.is_depth_test_enabled) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }
        }

        if(next_state.depth_func != current_state.depth_func) {
            glDepthFunc(next_state.depth_func);
        }

        if(next_state.polygon_offset_factor != current_state.polygon_offset_factor
           || next_state.polygon_offset_units != current_state.polygon_offset_units) {
            glPolygonOffset(next_state.polygon_offset_factor, next_state.polygon_offset_units);
        }

        if(next_state.is_stencil_write_enabled != current_state.is_stencil_write_enabled) {
            glStencilMask(next_state.is_stencil_write_enabled ? 0xFFFFFFFF : 0);
        }

        if(next_state.is_stencil_test_enabled != current_state.is_stencil_test_enabled) {
            if(next_state.is_stencil_test_enabled) {
                glEnable(GL_STENCIL_TEST);
            } else {
                glDisable(GL_STENCIL_TEST);
            }
        }

        if(next_state.frontface_state.fall_op != current_state.frontface_state.fall_op
                || next_state.frontface_state.pass_op != current_state.frontface_state.pass_op
                || next_state.frontface_state.depth_fail_op != current_state.frontface_state.depth_fail_op) {
            glStencilOpSeparate(GL_FRONT, next_state.frontface_state.fall_op, next_state.frontface_state.pass_op, next_state.frontface_state.depth_fail_op);
        }
        
        if(next_state.frontface_state.compare_op != current_state.frontface_state.compare_op
                || next_state.frontface_state.reference != current_state.frontface_state.reference
                || next_state.frontface_state.mask != current_state.frontface_state.mask) {
            glStencilFuncSeparate(GL_FRONT, next_state.frontface_state.compare_op, next_state.frontface_state.reference, next_state.frontface_state.mask);
        }

        if(next_state.backface_state.fall_op != current_state.backface_state.fall_op
           || next_state.backface_state.pass_op != current_state.backface_state.pass_op
           || next_state.backface_state.depth_fail_op != current_state.backface_state.depth_fail_op) {
            glStencilOpSeparate(GL_BACK, next_state.backface_state.fall_op, next_state.backface_state.pass_op, next_state.backface_state.depth_fail_op);
        }

        if(next_state.backface_state.compare_op != current_state.backface_state.compare_op
           || next_state.backface_state.reference != current_state.backface_state.reference
           || next_state.backface_state.mask != current_state.backface_state.mask) {
            glStencilFuncSeparate(GL_BACK, next_state.backface_state.compare_op, next_state.backface_state.reference, next_state.backface_state.mask);
        }

        if(next_state.is_color_write_enabled != current_state.is_color_write_enabled
                || next_state.is_alpha_write_enabled != current_state.is_alpha_write_enabled) {
            GLboolean color_enabled = next_state.is_color_write_enabled ? GL_TRUE : GL_FALSE;
            glColorMask(color_enabled, color_enabled, color_enabled, next_state.is_alpha_write_enabled);
        }

        if(next_state.is_alpha_to_coverage_enabled != current_state.is_alpha_to_coverage_enabled) {
            if(next_state.is_alpha_to_coverage_enabled) {
                glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            } else {
                glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
            }
        }

        if(next_state.program != current_state.program) {
            glUseProgram(next_state.program);
        }

        for(const auto& bound_texture : next_state.bound_textures) {
            glBindTextureUnit(bound_texture.first, bound_texture.second);
        }

        if(next_state.drawbuffer != current_state.drawbuffer) {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, next_state.drawbuffer);
        }

        current_state = next_state;
        next_state = {};
    }

    void open_gl_context::set_depth_func(GLenum depth_func) {
        next_state.depth_func = depth_func;
    }

    void open_gl_context::set_framebuffer(const framebuffer &fb) {
        next_state.drawbuffer = fb.get_gl_name();
    }

    void open_gl_context::set_blending_params(GLenum src_color, GLenum dst_color, GLenum src_alpha, GLenum dst_alpha) {
        next_state.src_color = src_color;
        next_state.dst_color = dst_color;
        next_state.src_alpha = src_alpha;
        next_state.dst_alpha = dst_alpha;
    }

    void open_gl_context::set_shader(GLuint program) {
        next_state.program = program;
    }

    void open_gl_context::set_polygon_offset(float factor, float units) {
        next_state.polygon_offset_factor = factor;
        next_state.polygon_offset_units = units;
    }
}
