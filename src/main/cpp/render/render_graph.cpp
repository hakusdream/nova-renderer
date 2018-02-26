/*!
 * \author ddubois 
 * \date 25-Feb-18.
 */

#include "render_graph.h"
#include <stdexcept>

namespace nova {
    render_graph::render_graph(std::unordered_map<std::string, render_pass> passes) : passes(passes) {
        /*
         * We want to compile the passes into something super useful
         *
         * Things to do:
         *  - Loop through the passes. Find out which passes write to each resource and which read from it. We can store
         *      this as a couple hash maps from string name of resource to string name of pass. Not hashing strings will
         *      probably be a good optimization to make
         * - All the passes that write to resource X must happen before all the passes that read from resource X
         *      - This is somewhat complicated by the fact that pass A can write to resource X and Y, then pass B uses X
         *          and writes to Z and C uses Y and Z... so if I just find what needs to happen before C I might get an
         *          execution order of B then A, even though that's wrong
         */

        /*
         * Build some accelleration structures
         */

        // Maps from resource name to pass that writes to that resource, then from resource name to pass that reads from
        // that resource
        resource_to_write_pass = std::unordered_map<std::string, std::vector<std::string>>{};
        resource_to_read_pass = std::unordered_map<std::string, std::vector<std::string>>{};

        for(const auto& item : passes) {
            auto& pass = item.second;
            for(const auto& input : pass.inputs) {
                resource_to_read_pass[input].push_back(pass.name);
            }

            for(const auto& output : pass.outputs) {
                resource_to_write_pass[output].push_back(pass.name);
            }
        }

        /*
         * Initial ordering of passes
         */

        // The passes, in simple dependency order
        if(resource_to_write_pass.find("backbuffer") == resource_to_write_pass.end()) {
            LOG(ERROR) << "This render graph does not write to the backbuffer. Unable to laod this shaderpack because it can't render anything";
            throw render_graph_validation_error("no backbuffer");

        } else {    // While the throw should make it clear that this is a separate branch, I forgot so here's an else
            auto backbuffer_writes = resource_to_read_pass["backbuffer"];
            ordered_passes.insert(ordered_passes.end(), backbuffer_writes.begin(), backbuffer_writes.end());

            add_dependent_passes(backbuffer_writes);
        }
    }

    void render_graph::add_dependent_passes(const std::vector<std::string> &passes_just_added) {
        auto passes_added = std::vector<std::string>{};
        for(const auto& pass_name : passes_just_added) {
            // See what this pass reads from and add the passes that write to those resources
            const auto& read_resources = passes[pass_name];
            for(const auto& resource_name : read_resources) {
                if(resource_to_write_pass.find(resource_name) == resource_to_write_pass.end()) {
                    // TODO: Ignore the implicitly defined resources
                    LOG(ERROR) << "Pass " << pass_name << " reads from resource " << resource_name << ", but nothing writes to it";

                } else {
                    const auto& write_passes = resource_to_write_pass[resource_name];

                    passes_added.insert(passes_added.end(), write_passes.begin(), write_passes.end());
                    ordered_passes.insert(passes_added.end(), write_passes.begin(), write_passes.end());
                }
            }
        }

        //
        add_dependent_passes(passes_added);
    }

    render_graph_validation_error::render_graph_validation_error(std::string& msg) : msg(msg) {}

    const char *render_graph_validation_error::what() {
        return msg.c_str();
    }
}
