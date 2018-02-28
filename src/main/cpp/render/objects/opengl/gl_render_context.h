/*!
 * \author ddubois 
 * \date 22-Feb-18.
 */

#ifndef RENDERER_GL_RENDER_CONTEXT_H
#define RENDERER_GL_RENDER_CONTEXT_H

namespace nova {
    /*!
     * \brief The rendering context for OpenGL
     * 
     * Keeps track of bound resources, only issuing GL calls if the resource has changed. You can call into the render
     * context to bind a texture to a given binding locaion all you want, but if it won't actually change render state 
     * then nothing's gonna happen
     */
    class gl_render_context {

    };
}


#endif //RENDERER_GL_RENDER_CONTEXT_H
