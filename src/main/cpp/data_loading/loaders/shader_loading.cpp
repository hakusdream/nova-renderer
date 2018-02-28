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
    // TODO: Fill this in
    std::vector<fs::path> bedrock_filenames = {};

    std::vector<fs::path> optifine_filenames = {
            fs::path("gbuffers_basic"),
            fs::path("gbuffers_textured"),
            fs::path("gbuffers_textured_lit"),
            fs::path("gbuffers_skybasic"),
            fs::path("gbuffers_skytextured"),
            fs::path("gbuffers_clouds"),
            fs::path("gbuffers_terrain"),
            fs::path("gbuffers_terrain_solid"),
            fs::path("gbuffers_terrain_cutout_mip"),
            fs::path("gbuffers_terrain_cutout"),
            fs::path("gbuffers_damagedblock"),
            fs::path("gbuffers_water"),
            fs::path("gbuffers_block"),
            fs::path("gbuffers_beaconbeam"),
            fs::path("gbuffers_item"),
            fs::path("gbuffers_entities"),
            fs::path("gbuffers_armor_glint"),
            fs::path("gbuffers_spidereyes"),
            fs::path("gbuffers_hand"),
            fs::path("gbuffers_weather"),
            fs::path("composite"),
            fs::path("composite1"),
            fs::path("composite2"),
            fs::path("composite3"),
            fs::path("composite4"),
            fs::path("composite5"),
            fs::path("composite6"),
            fs::path("composite7"),
            fs::path("final"),
            fs::path("shadow"),
            fs::path("shadow_solid"),
            fs::path("shadow_cutout"),
            fs::path("deferred"),
            fs::path("deferred1"),
            fs::path("deferred2"),
            fs::path("deferred3"),
            fs::path("deferred4"),
            fs::path("deferred5"),
            fs::path("deferred6"),
            fs::path("deferred7"),
            fs::path("gbuffers_hand_water"),
            fs::path("deferred_last"),
            fs::path("composite_last"),
    };

    bool contains_bedrock_files(std::vector<filesystem::path> &files);
    bool contains_optifine_files(std::vector<filesystem::path> &files);

    std::unordered_map<std::string, material> load_shaderpack(const std::string &shaderpack_name) {
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

            auto sources = load_sources_from_zip_file(shaderpack_name, {});
            return {};

        } else {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a regular folder";
            LOG(INFO) << "Loading passes";

            auto shaderpack_directory = fs::path("shaderpacks") / shaderpack_name;

            std::unordered_map<std::string, material> passes = load_passes_from_folder(shaderpack_directory);
            if(passes.empty()) {
                LOG(WARNING) << "No passes defines by shaderpack " << shaderpack_name << ". Attempting to guess the intended shaderpack format";

                auto files = get_shader_names_in_folder(shaderpack_directory / "shaders");

                if(contains_bedrock_files(files)) {
                    passes = parse_passes_from_json(get_default_bedrock_passes());

                } else if(contains_optifine_files(files)) {
                    passes = parse_passes_from_json(get_default_optifine_passes());

                } else {
                    LOG(FATAL) << "Cannot work with the format of this shaderpack. Please chose another one and try again";
                }
            } else {
                // TODO: Try to identify if the passes have at least one pass that matches either the default Bedrock
                // TODO: or Optifine setup
                // Right now I'm not dealing with that
            }

            LOG(INFO) << "Reading shaders from disk";
            auto sources = load_sources_from_folder(shaderpack_directory, passes);

            LOG(INFO) << "Compiling shaders";
            for(auto& named_pass : passes) {
                // TODO: Multithreaded shader compilation
                named_pass.second.program = std::make_shared<gl_shader_program>(sources[named_pass.first]);

                glObjectLabel(GL_PROGRAM, named_pass.second.program->gl_name, named_pass.first.size(), named_pass.first.c_str());
            }

            return passes;
        }
    }

    bool contains_bedrock_files(std::vector<filesystem::path> &files) {
        for(const auto &bedrock_name : bedrock_filenames) {
            if(find(files.begin(), files.end(), bedrock_name) != files.end()) {
                return true;
            }
        }
        return false;
    }

    bool contains_optifine_files(std::vector<filesystem::path> &files) {
        for(const auto& bedrock_name : bedrock_filenames) {
            if(find(files.begin(), files.end(), bedrock_name) != files.end()) {
                return true;
            }
        }
        return false;
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

    std::unordered_map<std::string, material> parse_passes_from_json(const nlohmann::json &shaders_json) {
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

            auto material = material(material_state_name, parent_state_name, json_node);
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
            fill_field(item.first, definition_map, &material::sampler_states);
            fill_field(item.first, definition_map, &material::textures);
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
            fill_field(item.first, definition_map, &material::dependencies);
            fill_field(item.first, definition_map, &material::texture_inputs);
            fill_field(item.first, definition_map, &material::texture_outputs);

            LOG(TRACE) << "Filed in all fields on material " << cur_state.name;
        }

        return definition_map;
    }
























































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
