/*! 
 * \author gold1 
 * \date 13-Jun-17.
 */

#include "shader_source_structs.h"
#include <easylogging++.h>

namespace nova {
    el::base::Writer& operator<<(el::base::Writer& out, const std::vector<shader_line>& lines) {
        for(const auto& line : lines) {
            out << line;
        }

        return out;
    }

    el::base::Writer& operator<<(el::base::Writer& out, const shader_line& line) {
        out << "\t" << line.line_num << "(" << line.shader_name << ") " << line.line << "\n";
        return out;
    }
}