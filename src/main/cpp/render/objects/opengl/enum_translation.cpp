/*!
 * \author ddubois 
 * \date 27-Feb-18.
 */

#include "enum_translation.h"

namespace nova {
    GLenum stencil_op_to_gl(const stencil_op_enum &op) {
        switch(op) {
            case stencil_op_enum::Decr:
                return GL_DECR;
            case stencil_op_enum::DecrWrap:
                return GL_DECR_WRAP;
            case stencil_op_enum::Incr:
                return GL_INCR;
            case stencil_op_enum::IncrWrap:
                return GL_INCR_WRAP;
            case stencil_op_enum::Invert:
                return GL_INVERT;
            case stencil_op_enum::Keep:
                return GL_KEEP;
            case stencil_op_enum::Replace:
                return GL_REPLACE;
            case stencil_op_enum::Zero:
                return GL_ZERO;
            default:
                LOG(ERROR) << "No conversion into an OpenGL constant for stencil op " << stencil_op_enum::to_string(op);
                return 0;
        }
    }

    GLenum compare_op_to_gl(const compare_op &op) {
        switch(op) {
            case compare_op::Never:
                return GL_NEVER;
            case compare_op::Less:
                return GL_LESS;
            case compare_op::LessEqual:
                return GL_LEQUAL;
            case compare_op::Greater:
                return GL_GREATER;
            case compare_op::GreaterEqual:
                return GL_GEQUAL;
            case compare_op::Equal:
                return GL_EQUAL;
            case compare_op::NotEqual:
                return GL_NOTEQUAL;
            case compare_op::Always:
                return GL_ALWAYS;
            default:
                LOG(ERROR) << "No conversion into an OpenGL constant for compare op " << compare_op::to_string(op);
                return 0;
        }
    }
}
