/*!
 * \author ddubois 
 * \date 26-Feb-18.
 */

#ifndef RENDERER_OPEN_GL_CONTEXT_H
#define RENDERER_OPEN_GL_CONTEXT_H

#include <unordered_map>
#include "../textures/texture2D.h"
#include "../framebuffer.h"
#include "../material.h"

namespace nova {
    struct stencil_test_state {
        GLenum fall_op;
        GLenum pass_op;
        GLenum depth_fail_op;

        GLenum compare_op;
        int reference;
        GLuint mask;
    };

    /*!
     * \brief All the state of the OpenGL 4.6 context, made available for you
     */
    struct opengl_state {
        bool is_blending_enabled;
        GLenum src_color;
        GLenum dst_color;
        GLenum src_alpha;
        GLenum dst_alpha;

        bool is_culling_eanbled;
        GLenum culling_mode;

        GLenum depth_func;
        float polygon_offset_factor;
        float polygon_offset_units;
        bool is_depth_write_enabled;
        bool is_depth_test_enabled;
        bool is_stencil_write_enabled;
        bool is_stencil_test_enabled;
        stencil_test_state frontface_state;
        stencil_test_state backface_state;

        bool is_color_write_enabled;
        bool is_alpha_write_enabled;

        bool is_alpha_to_coverage_enabled;

        std::unordered_map<GLuint, GLuint> bound_textures;
        GLuint drawbuffer;
        GLuint program;
    };

    /*!
     * \brief Manages OpenGL state by emulating the OpenGL state machine and only issuing OpenGL calls to change things
     * when they actually change. Meant as a layer between the render loop and the OpenGL context so that we don't
     * constantly bind texture x to texture unit y or something equally silly
     *
     * This class exposes a number of functions which mimic the setters for OpenGL state, such as methods to bind
     * textures, UBOs, shaders, etc. to the OpenGL context. This class will check to see if that bit of data is already
     * bound. If so, awesome! No ned to do anything else. If and only if the data is not bound will a GL call be issued
     *
     * This class maintains two copies of the OpenGL state: the current state and the next state. When you call methods
     * on this class it sets variables in the next state. Then, when you call commit, the parts of the new state that
     * were not set in the old state become actual OpenGL calls, and the new state is copied to the current state
     *
     * No validation is done on parameters to any of the methods here. If you pass in bad values you'll likely see
     * silent failures. All well.
     */
    class open_gl_context {
    public:
        /*!
         * \brief Sets the default values for all the rendering states
         *
         * Blending: Off
         * Culling: On, cull backfaces
         * Depth Write: On
         * Depth Test: On
         * Stencil Test: Off
         * Stencil Write: Off
         * Color Write: On
         * Alpha to Coverage: Off
         * Alpha Write: On
         */
        void set_default_state();

        /*!
         * \brief Sets blending to be either enables or disabled
         * \param enabled If true, enables blending. If false, does not
         */
        void set_blending_enabled(bool enabled);

        void set_blending_params(GLenum src_color, GLenum dst_color, GLenum src_alpha, GLenum dst_alpha);

        /*!
         * \brief Sets face culling to be either enabled or disabled
         * \param enabled If true, enables face culling. If false, disables it
         */
        void set_culling_enabled(bool enabled);

        /*!
         * \brief Sets the face culling mode
         * \param culling_mode Either GL_BACK or GL_FRONT, depending on which face you want to cull
         */
        void set_culling_mode(GLenum culling_mode);

        /*!
         * \brief Sets whether depth writing is enabled or not
         * \param enabled If true, depth writing is enabled. If false, it is not
         */
        void set_depth_write_enabled(bool enabled);

        /*!
         * \brief Sets whether the depth test is enabled or not
         * \param enabled If true, depth test is enabled. If false, it it not
         */
        void set_depth_test_enabled(bool enabled);

        /*!
         * \brief Sets whether writing to the stencil buffer is enabled or not
         * \param enabled If true, stencil writing is enabled. If false, it is not
         */
        void set_stencil_write_enabled(bool enabled);

        /*!
         * \brief Sets whether the stencil test is enabled or not
         * \param enabled If true, the stencil test is enabled. If false, it is not
         */
        void set_stencil_test_enabled(bool enabled);

        /*!
         * \brief Set the stencil ops for a face group
         *
         * See https://www.khronos.org/opengl/wiki/GLAPI/glStencilOpSeparate for more docs
         *
         * \param face Whether to set this for front faces, back faces, or both
         * \param fail_op What to do when the stencil test fails
         * \param pass_op What to do when the stencil test passes but the depth test fails
         * \param depth_fail_op What to do when the stencil text fails and the depth test fails;
         */
        void set_stencil_op_separate(int face, int fail_op, int pass_op, int depth_fail_op);

        /*!
         * \brief Set the stencil test parameters for a face group
         *
         * See https://www.khronos.org/opengl/wiki/GLAPI/glStencilFuncSeparate for more docs
         *
         * \param face Whether to set this for the front faces, back faces, or both
         * \param compare_op The operation to use to perform the stencil test
         * \param reference The reference value to compare the stencil buffer value against
         * \param mask A bitwise mask to apply to both the stencil buffer value and the reference value
         */
        void set_stencil_func_separate(int face, int compare_op, int reference, int mask);

        /*!
         * \brief Sets whether writing to the color buffer is enabled or not
         * \param enabled If true, color writes are enabled. If false, they are not
         */
        void set_color_write_enabled(bool enabled);

        /*!
         * \brief Sets whether alpha to coverage is enabled or not
         * \param enabled If true, alpha to coverage is enabled. If false, it is not
         */
        void set_alpha_write_enabled(bool enabled);

        /*!
         * \brief Sets whether alpha to coverage is enabled or not
         * \param enabled If true, alpha-to-coverage is enabled. If false, it is not
         */
        void set_alpha_to_coverage_enabled(bool enabled);

        void set_shader(GLuint program);

        /*!
         * \brief Binds a texture to a texture unit
         *
         * \param texture The texture to bind
         * \param texture_unit The texture unit to bind it to
         */
        void bind_texture(const texture2D &texture, uint32_t texture_unit);

        void set_framebuffer(const framebuffer &fb);

        void commit();

        void set_depth_func(GLenum depth_func);

        void set_polygon_offset(float factor, float units);

    private:
        opengl_state current_state;
        opengl_state next_state;
    };
}

#endif //RENDERER_OPEN_GL_CONTEXT_H
