/*!
 * \author gold1
 * \date 01-Jun-17.
 */

#ifndef RENDERER_FRAMEBUFFER_H
#define RENDERER_FRAMEBUFFER_H

#include <glad/glad.h>
#include <vector>
#include <unordered_map>
#include <set>
#include "textures/texture2D.h"

namespace nova {
    /*!
     * \brief A framebuffer object
     *
     * Framebuffers may have multiple attachments, and each attachment may be enabled for writing. All attachments are
     * enabled for reading all the time
     */
    class framebuffer {
    public:
        framebuffer();

        framebuffer(framebuffer &&other) noexcept;

        ~framebuffer();

        void add_color_attachment(GLuint binding, GLuint tex_name);

        void generate_mipmaps();

        void set_depth_buffer(GLuint depth_buffer);

        GLuint get_gl_name() const;

    private:
        GLuint framebuffer_id;

        /*!
         * \brief All the color attachments that this framebuffer has
         *
         * These attachments are stored in a map from the index in a shader (0, 1, 2...) to their OpenGL name. This allows
         * easy operations on the attachments (such as
         */
        std::unordered_map<int, GLuint> color_attachments_map;

        std::set<GLenum> drawbuffers;

        GLuint* color_attachments;
        bool has_depth_buffer = false;

        void check_status();
    };
}

#endif //RENDERER_FRAMEBUFFER_H
