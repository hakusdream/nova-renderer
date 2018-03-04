/*!
 * \brief Implements fucntions for loading a shaderpack
 *
 * The functions here work for both zip and folder shaderpacks
 *
 * \author ddubois 
 * \date 03-Sep-16.
 */

#include <easylogging++.h>

#include "loaders.h"
#include "shader_loading.h"
#include "loader_utils.h"
#include "../../utils/utils.h"

namespace nova {
    shaderpack_data load_shaderpack(const std::string &shaderpack_name) {
        // Load the passes
        //  - Check if there's passes in the shaderpack
        //  - If so, identify if there are a complete set of passes
        //      - If there are not, fill in missing passes from the defaults
        //          - If at least one of the pass filenames matches one of the filenames of the default Bedrock
        //              materials, load the Bedrock passes as the default passes
        //          - If at least one of the pass filenames matches one of the filenames of the default Optifine
        //              passes, load the Optifine passes as the default passes
        //          - If all filenames are unique, there are no default passes because the passes
        //  - If there are no passes, check the shader names
        //      - If all the shader names match Bedrock shader names, load the Bedrock passes as the default passes
        //      - If all the shader names match Optifine shader names, load the Optifine passes as the default passes
        //      - If some of the shader names match Bedrock shader names, load the Bedrock passes as the default passes
        //          and print a warning (they might be including the files in their other files)
        //      - If some of the shader names match Bedrock shader names, load the Optifine passes as the default passes
        //          and print a warning (they might be including the files in their other files)
        //      - If none of the shader names match known shader names, and there's no passes, thenw e don't know how to
        //          handle this. Print an error, pop up an error on screen about "this shaderpack can't be loaded" and
        //          make the user chose something else

        LOG(INFO) << "Loading shaderpack " << shaderpack_name;
        auto pack = shaderpack_data{};

        if(is_zip_file(shaderpack_name)) {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a zip file";

            auto sources = load_sources_from_zip_file(shaderpack_name, {});
            return {};

        } else {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a regular folder";

            auto shaderpack_directory = fs::path("shaderpacks") / shaderpack_name;

            LOG(INFO) << "Loading materials";
            pack.materials_by_pass = load_materials_from_folder(shaderpack_directory);
            LOG(INFO) << "Loading passes";
            pack.passes = load_passes_from_folder(shaderpack_directory);
            LOG(INFO) << "Loading dynamic textures";
            pack.dynamic_textures = load_texture_definitions_from_folder(shaderpack_directory);
        }

        LOG(INFO) << "All data for shaderpack " << shaderpack_name << " read from disk";

        return pack;
    }

    template<typename Type>
    void fill_in_material_state_field(const std::string& our_name, std::unordered_map<std::string, material>& all_materials, std::function<optional<Type>&(material&)> get_field_from_material) {
        auto &us = all_materials[our_name];
        auto &cur_state = us;
        bool value_found = (bool) get_field_from_material(us);

        while(!value_found) {
            const auto &parent_name = cur_state.parent_name;
            if(parent_name) {
                cur_state = all_materials[parent_name.value()];
                auto field_value = get_field_from_material(cur_state);
                if(field_value) {
                    get_field_from_material(us) = field_value.value();
                    value_found = true;
                }

            } else {
                break;
            }
        }
    }

    template<typename Type>
    void fill_field(const std::string& name, std::unordered_map<std::string, material> materials, optional<Type> material::* ptr) {
        fill_in_material_state_field<Type>(name, materials, [ptr](material& s) -> optional<Type>&{ return s.*ptr; });
    }

    std::vector<material> parse_materials_from_json(const nlohmann::json &shaders_json) {
        std::unordered_map<std::string, material> definition_map;
        for(auto itr = shaders_json.begin(); itr != shaders_json.end(); ++itr) {
            auto material_state_name = itr.key();
            auto json_node = itr.value();
            optional<std::string> parent_state_name = optional<std::string>{};

            int colon_pos = material_state_name.find(':');
            if(colon_pos != std::string::npos) {
                auto parent_name = material_state_name.substr(colon_pos + 1);
                parent_state_name = parent_name;
                material_state_name = material_state_name.substr(0, colon_pos);
            }

            auto mat = material(material_state_name, parent_state_name, json_node);
            definition_map[material_state_name] = mat;
            LOG(TRACE) << "Inserted a material named " << material_state_name;
        }

        auto materials = std::vector<material>{};

        // I don't really know the O(n) for this thing. It's at least O(n) and probs O(nlogn) but someone mathy can
        // figure it out
        for(const auto& item : definition_map) {
            auto &cur_state = item.second;

            if(!cur_state.parent_name) {
                // No parent? I guess we get what we have then
                continue;
            }

            fill_field(item.first, definition_map, &material::pass);
            fill_field(item.first, definition_map, &material::defines);
            fill_field(item.first, definition_map, &material::states);
            fill_field(item.first, definition_map, &material::vertex_shader);
            fill_field(item.first, definition_map, &material::fragment_shader);
            fill_field(item.first, definition_map, &material::geometry_shader);
            fill_field(item.first, definition_map, &material::tessellation_evaluation_shader);
            fill_field(item.first, definition_map, &material::tessellation_control_shader);
            fill_field(item.first, definition_map, &material::vertex_fields);
            fill_field(item.first, definition_map, &material::front_face);
            fill_field(item.first, definition_map, &material::back_face);
            fill_field(item.first, definition_map, &material::input_textures);
            fill_field(item.first, definition_map, &material::output_textures);
            fill_field(item.first, definition_map, &material::filters);
            fill_field(item.first, definition_map, &material::fallback);
            fill_field(item.first, definition_map, &material::depth_bias);
            fill_field(item.first, definition_map, &material::slope_scaled_depth_bias);
            fill_field(item.first, definition_map, &material::stencil_ref);
            fill_field(item.first, definition_map, &material::stencil_read_mask);
            fill_field(item.first, definition_map, &material::stencil_write_mask);
            fill_field(item.first, definition_map, &material::msaa_support);
            fill_field(item.first, definition_map, &material::primitive_mode);
            fill_field(item.first, definition_map, &material::source_blend_factor);
            fill_field(item.first, definition_map, &material::destination_blend_factor);
            fill_field(item.first, definition_map, &material::alpha_src);
            fill_field(item.first, definition_map, &material::alpha_dst);
            fill_field(item.first, definition_map, &material::depth_func);
            fill_field(item.first, definition_map, &material::render_queue);

            LOG(TRACE) << "Filled in all fields on material " << cur_state.name;

            materials.push_back(cur_state);
        }

        return materials;
    }

    std::unordered_map<std::string, render_pass> parse_passes_from_json(const nlohmann::json& json) {
        auto passes = std::unordered_map<std::string, render_pass>{};

        for(const auto& pass_json : json) {
            auto pass = render_pass(pass_json);
            passes[pass.name] = pass;
        }

        return passes;
    }

    std::unordered_map<std::string, texture_resource> parse_textures_from_json(nlohmann::json& json) {
        auto textures = std::unordered_map<std::string, texture_resource>{};

        for(const auto& texture_json : json) {
            auto texture = texture_resource(texture_json);
            textures[texture.name] = texture;
        }

        return textures;
    };

    std::string get_filename_from_include(const std::string& include_line) {
        auto quote_pos = include_line.find('"');
        return include_line.substr(quote_pos + 1, include_line.size() - quote_pos - 2);
    }

    std::unordered_map<std::string, shader_definition> load_sources_from_zip_file(const std::string &shaderpack_name, const std::vector<std::string> &shader_names) {
        LOG(FATAL) << "Cannot load zipped shaderpack " << shaderpack_name;
        throw std::runtime_error("Zipped shaderpacks not yet supported");
    }

    nlohmann::json& get_default_bedrock_passes() {
        static nlohmann::json default_bedrock_passes;

        if(default_bedrock_passes.empty()) {
            std::ifstream default_json_file("config/default/bedrock_passes.json");
            if(default_json_file.is_open()) {
                default_bedrock_passes = load_json_from_stream(default_json_file);
            } else {
                LOG(ERROR) << "Could not open config/default/bedrock_passes.json. Please download it from https://raw.githubusercontent.com/NovaMods/nova-renderer/master/jars/config/shaders.json";
            }
        }

        return default_bedrock_passes;
    }

    nlohmann::json& get_default_optifine_passes() {
        static nlohmann::json default_optifine_passes;

        if(default_optifine_passes.empty()) {
            std::ifstream default_json_file("config/default/bedrock_passes.json");
            if(default_json_file.is_open()) {
                default_optifine_passes = load_json_from_stream(default_json_file);
            } else {
                LOG(ERROR) << "Could not open config/default/bedrock_passes.json. Please download it from https://raw.githubusercontent.com/NovaMods/nova-renderer/master/jars/config/shaders.json";
            }
        }

        return default_optifine_passes;
    }
}
