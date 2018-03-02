/*!
 * \brief Defines a bunch of function and structs to help with loading shaders
 *
 * \author ddubois 
 * \date 04-Sep-16.
 */

#ifndef RENDERER_SHADER_LOADING_H_H
#define RENDERER_SHADER_LOADING_H_H

#include <experimental/filesystem>
#include <string>
#include <vector>
#include <unordered_map>

#include "shader_source_structs.h"
#include "../../render/objects/material.h"

namespace fs = std::experimental::filesystem;

namespace nova {
    /*!
     * \brief A shaderpack loaded from disk
     *
     * A shaderpack has
     * - A list of render passes. Render passes have:
     *      - The dynamic resources that this pass needs as an input
     *      - The dynamic resources that this pass writes to
     * - A list of materials. A material has
     *      - A fragment shader and vertex shader, and optionally a geometry shader and tessellation shader
     *      - All sorts of renderer state like depth test, stencil test, etc
     *      - The textures and UBOs the shader uses, and what location to bind them to
     *      - The textures in the framebuffer that this shader writes to, and where to bind them
     *      - The name of the pass that should render this material
     *      - A filter that selects geometry that this shader renders
     * - A list of dynamic resources. A dynamic resource is a texture or buffer that is written to by a shader. Nova 1
     *  only supports textures (unless that changes). A dynamic texture has:
     *      - A name
     *      - A resolution (relative to the screen, or in absolute pixels)
     *      - A format
     */
    struct shaderpack {
        std::unordered_map<std::string, material> materials_by_pass;
        std::vector<render_pass> ordered_passes;
        std::unordered_map<std::string, texture_resource> textures;
    };

    /*!
     * \brief Loads all the passes that are present in the shaderpack with the given zip file name
     *
     * \param shaderpack_name The name of the shaderpack file to load things from
     * \return A map from pass name to pass of all the passes found in that zip
     */
    std::unordered_map<std::string, material> load_passes_from_zip(const std::string& shaderpack_name);

    /*!
     * \brief Loads the source file of all the shaders with the provided names
     *
     * This will only work if the shaderpack is a zip file. If the shaderpack is just a folder, this will probably
     * fail in strange ways
     *
     * \param shaderpack_name The name of the shaderpack to load the shaders from
     * \param shader_names The list of names of shaders to load
     * \return A map from shader name to shader source
     */
    std::unordered_map<std::string, shader_definition> load_sources_from_zip_file(const std::string &shaderpack_name, const std::vector<std::string> &shader_names);

    /*!
     * \brief Loads all the passes that are present in the shaderpack with the given folder name
     *
     * \param shaderpack_path The name of the shaderpack folder to load things from
     * \return A map from pass name to pass of all the passes found in that folder
     */
    std::unordered_map<std::string, material> load_passes_from_folder(const fs::path& shaderpack_path);

    std::unordered_map<std::string, material> parse_passes_from_json(const nlohmann::json &shaders_json);

    /*!
     * \brief Gets a list of all the files in the given folder
     *
     * \param shaderpack_name The name of the shaderpack to get the names of the shaders in
     * \return The names of all the shaders in
     */
    std::vector<fs::path> get_shader_names_in_folder(const fs::path& shaderpack_path);

    /*!
     * \brief Loads the source file of all the shaders with the provided names
     *
     * This will only work if the shaderpack is a folder. If the shaderpack is a zip folder, this will probably fail
     * in a way I didn't explicitly anticipate
     *
     * \param shaderpack_name The name of the shaderpack to load the shaders from
     * \param passes The list of names of shaders to load
     * \return The loaded shaderpack
     */
    std::unordered_map<std::string, shader_definition> load_sources_from_folder(const fs::path &shaderpack_name, const std::unordered_map<std::string, material> &passes);

    /*!
     * \brief Extracts the filename from the #include line
     *
     * #include lines look like
     *
     * #include "shader.glsl"
     *
     * I need to get the bit inside the quotes
     *
     * \param include_line The line with the #include statement on it
     * \return The filename that is being included
     */
    std::string get_filename_from_include(const std::string& include_line);

    /*!
     * \brief Loads the JSON for the default Bedrock material files
     *
     * \return The JSON for the default Bedrock material files
     */
    nlohmann::json& get_default_bedrock_passes();

    /*!
     * \brief Loads the JSON for the default Optifine passes
     *
     * \return The JSON for the default Optifine passes
     */
    nlohmann::json& get_default_optifine_passes();
}

#endif //RENDERER_SHADER_LOADING_H_H
