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
    shaderpack load_shaderpack(const std::string &shaderpack_name) {
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

        if(is_zip_file(shaderpack_name)) {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a zip file";

            return load_sources_from_zip_file(shaderpack_name, {});

        } else {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a regular folder";

            auto shaderpack_directory = fs::path("shaderpacks") / shaderpack_name;

            auto passes = load_passes_from_folder(shaderpack_directory);
            if(passes.empty()) {
                LOG(WARNING) << "No passes defines by shaderpack " << shaderpack_name << ". Attempting to guess the intended shaderpack format";

                auto files = get_shader_names_in_folder(shaderpack_name);
            }

            return load_sources_from_folder(shaderpack_name, {});
        }
    }

    template<typename Type>
    void fill_in_material_state_field(const std::string& our_name, std::unordered_map<std::string, pass>& all_materials, std::function<optional<Type>&(pass&)> get_field_from_material) {
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
    void fill_field(const std::string& name, std::unordered_map<std::string, pass> materials, optional<Type> pass::* ptr) {
        fill_in_material_state_field<Type>(name, materials, [ptr](pass& s) -> optional<Type>&{ return s.*ptr; });
    }

    std::unordered_map<std::string, pass> get_material_definitions(const nlohmann::json &shaders_json) {
        std::unordered_map<std::string, pass> definition_map;
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

            auto material = pass(material_state_name, parent_state_name, json_node);
            definition_map[material_state_name] = material;
            LOG(TRACE) << "Inserted a material named " << material_state_name;
        }

        // I don't really know the O(n) for this thing. It's at least O(n) and probs O(nlogn) but someone mathy can
        // figure it out
        for(const auto& item : definition_map) {
            auto& cur_state = item.second;

            if(!cur_state.parent_name) {
                // No parent? I guess we get what we have then
                continue;
            }

            fill_field(item.first, definition_map, &pass::defines);
            fill_field(item.first, definition_map, &pass::states);
            fill_field(item.first, definition_map, &pass::vertex_shader);
            fill_field(item.first, definition_map, &pass::fragment_shader);
            fill_field(item.first, definition_map, &pass::geometry_shader);
            fill_field(item.first, definition_map, &pass::tessellation_evaluation_shader);
            fill_field(item.first, definition_map, &pass::tessellation_control_shader);
            fill_field(item.first, definition_map, &pass::vertex_fields);
            fill_field(item.first, definition_map, &pass::front_face);
            fill_field(item.first, definition_map, &pass::back_face);
            fill_field(item.first, definition_map, &pass::sampler_states);
            fill_field(item.first, definition_map, &pass::textures);
            fill_field(item.first, definition_map, &pass::filters);
            fill_field(item.first, definition_map, &pass::fallback);
            fill_field(item.first, definition_map, &pass::depth_bias);
            fill_field(item.first, definition_map, &pass::slope_scaled_depth_bias);
            fill_field(item.first, definition_map, &pass::stencil_ref);
            fill_field(item.first, definition_map, &pass::stencil_read_mask);
            fill_field(item.first, definition_map, &pass::stencil_write_mask);
            fill_field(item.first, definition_map, &pass::msaa_support);
            fill_field(item.first, definition_map, &pass::primitive_mode);
            fill_field(item.first, definition_map, &pass::source_blend_factor);
            fill_field(item.first, definition_map, &pass::destination_blend_factor);
            fill_field(item.first, definition_map, &pass::alpha_src);
            fill_field(item.first, definition_map, &pass::alpha_dst);
            fill_field(item.first, definition_map, &pass::depth_func);
            fill_field(item.first, definition_map, &pass::render_queue);
            fill_field(item.first, definition_map, &pass::dependencies);
            fill_field(item.first, definition_map, &pass::texture_inputs);
            fill_field(item.first, definition_map, &pass::texture_outputs);

            LOG(TRACE) << "Filed in all fields on material " << cur_state.name;
        }

        return definition_map;
    }





















































    std::vector<shader_definition> get_shader_definitions(nlohmann::json &shaders_json) {
        // Check if the top-level element is an array or an object. If it's
        // an array, load all the elements of the array into shader
        // definition objects. If it's an object, look for a property
        // called 'shaders' which should be an array. Load the definitions
        // from there.

        nlohmann::json &definitions_array = shaders_json;
        if(shaders_json.is_object()) {
            definitions_array = shaders_json["shaders"];
        }

        std::vector<shader_definition> definitions;
        for(auto& definition : definitions_array) {
            definitions.push_back(shader_definition(definition));
        }

        return definitions;
    }

    void warn_for_missing_fallbacks(std::vector<shader_definition> sources) {
        // Verify that all the fallbacks exist
        for(auto def : sources) {
            if(def.fallback_name) {
                bool found_fallback = false;
                for(auto test : sources) {
                    if(test.name == *def.fallback_name) {
                        found_fallback = true;
                    }
                }

                if(!found_fallback) {
                    LOG(WARNING) << "Could not find fallback shader " << *def.fallback_name << " for shader " << def.name
                                 << ".";
                }
            } else {
                LOG(WARNING) << "No fallback specified for shader " << def.name
                             << ". If you forget that shader, its geometry won't render";
            }
        }
    }

    /*!
     * \brief Lops off everything after the first '/' in the file path, returning only the bit before it with the
     * slash included
     *
     * \param file_path The file path to lop the name off of
     * \return The path to the folder that the provided file resides in
     */
    auto get_file_path(const std::string file_path) {
        auto slash_pos = file_path.find_last_of('/');
        return file_path.substr(0, slash_pos + 1);
    }

    /*!
     * \brief Gets the name of the shaderpack from the file path
     *
     * \param file_path The file path to get the shaderpack name from
     * \return The name of the shaderpack
     */
    auto get_shaderpack_name(const std::string &file_path) {
        auto slash_pos = file_path.find('/');
        auto afterShaderpacks=file_path.substr(slash_pos+1,file_path.size());
        auto new_slash_pos = afterShaderpacks.find('/');
        return afterShaderpacks.substr(0,new_slash_pos);
    }

    std::string get_filename_from_include(const std::string include_line) {
        auto quote_pos = include_line.find('"');
        return include_line.substr(quote_pos + 1, include_line.size() - quote_pos - 2);
    }

    auto get_included_file_path(const std::string &shader_path, const std::string &included_file_name) {
        if(included_file_name[0] == '/') {
            
            // This is an absolute include and it should be relative to the root directory
            //LOG(INFO) << "Loading include file path " << shader_path;
            auto shaderpack_name = get_shaderpack_name(shader_path);
            //LOG(INFO) << "Loading include file 1 name" << shaderpack_name;
            //LOG(INFO) << "Loading include file 1" << (shaderpack_name + "/shaders" + included_file_name);
            return "shaderpacks/"+shaderpack_name + "/shaders" + included_file_name;

        } else {
            // The include file is a relative include, this one's actually simpler
            auto folder_name = get_file_path(shader_path);
            //LOG(INFO) << "Loading include file 2" << (folder_name + included_file_name);
            return folder_name + included_file_name;
        }
    }

    std::vector<shader_line> load_shader_file(const std::string &shader_path, const std::vector<std::string> &extensions) {
        for(auto &extension : extensions) {
            auto full_shader_path = shader_path + extension;
            LOG(TRACE) << "Trying to load shader file " << full_shader_path;

            std::ifstream stream(full_shader_path, std::ios::in);
            if(stream.good()) {
                LOG(INFO) << "Loading shader file " << full_shader_path;
                return read_shader_stream(stream, full_shader_path);
            } else {
                LOG(WARNING) << "Could not read file " << full_shader_path;
            }
        }

        throw resource_not_found(shader_path);
    }

    std::vector<shader_line> read_shader_stream(std::istream &stream, const std::string &shader_path) {
        std::vector<shader_line> file_source;
        std::string line;
        auto line_counter = 1;
        while(std::getline(stream, line, '\n')) {
            if(line.find("#include") == 0) { 
                auto included_file = load_included_file(shader_path, line);
                file_source.insert(file_source.end(), std::begin(included_file), std::end(included_file));

            } else {
                file_source.push_back({line_counter, shader_path, line});
            }

            line_counter++;
        }

        return file_source;
    }

    std::vector<shader_line> load_included_file(const std::string &shader_path, const std::string &line) {
        auto included_file_name = get_filename_from_include(line);
        auto file_to_include = get_included_file_path(shader_path, included_file_name);
        LOG(TRACE) << "Dealing with included file " << file_to_include;

        try {
            return load_shader_file(file_to_include, {""});
        } catch(resource_not_found& e) {
            throw std::runtime_error("Could not load included file " + file_to_include);
        }
    }

    shaderpack load_sources_from_zip_file(const std::string &shaderpack_name, const std::vector<std::string> &shader_names) {
        LOG(FATAL) << "Cannot load zipped shaderpack " << shaderpack_name;
        throw std::runtime_error("Zipped shaderpacks not yet supported");
    }

    nlohmann::json& get_default_shaders_json() {
        static nlohmann::json default_shaders_json;

        if(default_shaders_json.empty()) {
            std::ifstream default_json_file("config/shaders.json");
            if(default_json_file.is_open()) {
                //default_json_file >> default_shaders_json;
                default_shaders_json = load_json_from_stream(default_json_file);
            } else {
                LOG(ERROR) << "Could not open the default shader.json file from the config folder. Please download it from https://raw.githubusercontent.com/NovaMods/nova-renderer/master/jars/config/shaders.json";
            }
        }

        return default_shaders_json;
    }
}
