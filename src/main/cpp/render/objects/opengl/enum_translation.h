/*!
 * \brief Function to translate enums from the internal format to OpenGL format
 * \author ddubois 
 * \date 27-Feb-18.
 */

#ifndef RENDERER_ENUM_TRANSLATION_H
#define RENDERER_ENUM_TRANSLATION_H

#include <glad/glad.h>
#include "../material.h"

namespace nova {
    /*!
     * \brief Translates a Nova internal stencil op to the corresponding OpenGL constant
     * \param op The Nova internal stencil operation to translate
     * \return The OpenGL constant that corresponds to the translated stencil op
     */
    GLenum stencil_op_to_gl(const stencil_op_enum &op);

    /*!
     * \brief Translates
     * \param op
     * \return
     */
    GLenum compare_op_to_gl(const compare_op &op);
}


#endif //RENDERER_ENUM_TRANSLATION_H
