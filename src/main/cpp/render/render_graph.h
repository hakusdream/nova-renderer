/*!
 * \author ddubois 
 * \date 25-Feb-18.
 */

#ifndef RENDERER_RENDER_GRAPH_H
#define RENDERER_RENDER_GRAPH_H

#include "objects/render_pass.h"
#include "objects/textures/texture_manager.h"

namespace nova {
    class render_graph_validation_error : public std::exception {
    public:
        explicit render_graph_validation_error(std::string& msg);
        virtual const char* what();
    private:
        std::string msg;
    };

    /*!
    * \brief A render graph has all the passes defined by a shaderpack
    */
    class render_graph {
    public:
        /*!
         * \brief Initializes this render graph with the provided passes, compiling the passes into the final execution
         * order
         * \param passes The render passes that this render graph is build from
         * \throws render_graph_validation_error if there's an error validating this render graph
         */
        explicit render_graph(std::unordered_map<std::string, render_pass> passes);

        void compile();

    private:
        std::unordered_map<std::string, render_pass> passes;

        /*!
         * \brief Adds all the passes that are dependent on at least one of the passes in `passes_just_added` to the
         * list of ordered passes
         *
         * \param passes_just_added The passes that were just added to the list of ordered passes
         */
        void add_dependent_passes(const std::vector<std::string> &passes_just_added);

        std::vector<std::string> ordered_passes;
        std::unordered_map<std::string, std::vector<std::string>> resource_to_write_pass;
        std::unordered_map<std::string, std::vector<std::string>> resource_to_read_pass;
    };
}

#endif //RENDERER_RENDER_GRAPH_H
