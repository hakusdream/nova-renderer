//
// Created by gold1 on 01-Jun-17.
//

#include "framebuffer.h"
#include <easylogging++.h>

namespace nova {
    /* framebuffer */

    framebuffer::framebuffer() {
        glCreateFramebuffers(1, &framebuffer_id);
    }

    void framebuffer::add_color_attachment(const GLuint binding, const GLuint tex_name) {
        glNamedFramebufferTexture(framebuffer_id, GL_COLOR_ATTACHMENT0 + binding, tex_name, 0);
        color_attachments_map[binding] = tex_name;
    }

    framebuffer::framebuffer(framebuffer &&other) {
        framebuffer_id = other.framebuffer_id;
        other.framebuffer_id = 0;
    }

    framebuffer::~framebuffer() {
        glDeleteFramebuffers(1, &framebuffer_id);
    }

    void framebuffer::set_depth_buffer(GLuint depth_buffer) {
        glNamedFramebufferTexture(framebuffer_id, GL_DEPTH_ATTACHMENT, depth_buffer, 0);
        has_depth_buffer = true;

        check_status();
    }

    void framebuffer::check_status() {
        auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        switch(status) {
            case GL_FRAMEBUFFER_COMPLETE:
                LOG(DEBUG) << "Framebuffer " << framebuffer_id << " is complete";
                break;
            case GL_FRAMEBUFFER_UNDEFINED:
                LOG(ERROR) << "Somehow didn't bind the framebuffer";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                LOG(ERROR) << "A necessary attachment is not initialized";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                LOG(ERROR) << "There are no images attached to the framebuffer";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                LOG(ERROR) << "Every drawing buffer has an attachment";
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                LOG(ERROR) << "The framebuffrer format is not supported";
                break;
        }
    }

    void framebuffer::bind() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_id);
    }

    void framebuffer::generate_mipmaps() {
        for(const auto &item : color_attachments_map) {
            glGenerateTextureMipmap(item.second);
        }
    }
}
