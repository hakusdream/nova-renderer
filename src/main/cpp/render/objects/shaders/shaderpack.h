/*!
 * \brief
 *
 * \author ddubois 
 * \date 20-Sep-16.
 */

#ifndef RENDERER_SHADER_INTERFACE_H
#define RENDERER_SHADER_INTERFACE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <initializer_list>
#include <mutex>
#include <optional.hpp>

#include "pipeline_creation.h"

namespace nova {
    class shader_definition;
    class renderpass;

    /*!
     * \brief Represents a shaderpack in all of its glory, along with some meta information about the options that this
     * shaderpack sets
     */
    class shaderpack {
    public:
        /*!
         * \brief Constructs a new shaderpack from the provided shader definitions
         * \param shaderpack_name The name of the shaderpcack to load
         */
        shaderpack(const std::string &name, std::vector<std::pair<material_state, shader_definition>>& shaders, const vk::RenderPass our_renderpass, std::shared_ptr<render_context> context, std::shared_ptr<shader_resource_manager> shader_resources, glm::ivec2& window_size);

        vk_shader_program &operator[](std::string key);

        vk_shader_program& get_shader(std::string key);

		std::unordered_map<std::string, vk_shader_program> &get_loaded_shaders();

        std::string& get_name();

    private:
        std::unordered_map<std::string, vk_shader_program> loaded_shaders;

        std::string name;

        /*!
         * \brief The indices of the framebuffer attachments that any of the non-shadow shaders write to
         */
        std::vector<unsigned int> all_drawbuffers;

        /*!
         * \brief The indices of all the framebuffer attachments that any of the shadow shaders write to
         */
        std::vector<unsigned int> shadow_drawbuffers;

        /*!
         * \brief The options that the shaders in this shaderpack set
         */
        nlohmann::json options;
        vk::Device device;
    };
}

#endif //RENDERER_SHADER_INTERFACE_H.
