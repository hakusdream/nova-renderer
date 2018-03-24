/*!
 * \brief Implements the functions in shader_loading.h
 *
 * \author ddubois 
 * \date 03-Sep-16.
 */

#include <easylogging++.h>
#include <shaderc/shaderc.hpp>
#include <experimental/filesystem>
#include <sstream>

#include "loaders.h"
#include "shader_loading.h"
#include "loader_utils.h"
#include "../../utils/utils.h"

namespace nova {
    /*!
     * \brief Holds the name of all the shaders to load
     *
     * As we add more shaders, we should add their names to this vector
     */
    std::vector<std::string> shader_names = {
            "gui"
    };

    std::vector<std::string> fragment_extensions = {
            ".fsh",
            ".frag",
            ".frag.spv"
    };

    std::vector<std::string> vertex_extensions = {
            ".vsh",
            ".vert",
            ".vert.spv"
    };

    std::vector<std::pair<material_state, shader_definition>> load_shaderpack(const std::string &shaderpack_name) {
        LOG(DEBUG) << "Loading shaderpack " << shaderpack_name;
        auto shader_sources = std::unordered_map<std::string, shader_definition>{};
        if(is_zip_file(shaderpack_name)) {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a zip file";
            return load_sources_from_zip_file(shaderpack_name, shader_names);

        } else {
            LOG(TRACE) << "Loading shaderpack " << shaderpack_name << " from a regular folder";
            return load_sources_from_folder(shaderpack_name, shader_names);
        }
    }

    template<typename Type>
    void fill_in_material_state_field(const std::string& our_name, std::unordered_map<std::string, material_state>& all_materials, std::function<optional<Type>&(material_state&)> get_field_from_material) {
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
    void fill_field(const std::string& name, std::unordered_map<std::string, material_state> materials, optional<Type> material_state::* ptr) {
        fill_in_material_state_field<Type>(name, materials, [ptr](material_state& s) -> optional<Type>&{ return s.*ptr; });
    }

    std::vector<material_state> get_material_definitions(const nlohmann::json &shaders_json) {
        std::unordered_map<std::string, material_state> definition_map;
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

            auto material = create_material_from_json(material_state_name, parent_state_name, json_node);
            definition_map[material_state_name] = material;
            LOG(TRACE) << "Inserted a material named " << material_state_name;
        }

        std::vector<material_state> definitions;

        // I don't really know the O(n) for this thing. It's at least O(n) and probs O(nlogn) but someone mathy can
        // figure it out
        for(const auto& item : definition_map) {
            auto& cur_state = item.second;

            definitions.push_back(cur_state);

            if(!cur_state.parent_name) {
                // No parent? I guess we get what we have then
                continue;
            }

            fill_field(item.first, definition_map, &material_state::defines);
            fill_field(item.first, definition_map, &material_state::states);
            fill_field(item.first, definition_map, &material_state::pass_index);
            fill_field(item.first, definition_map, &material_state::outputs);
            fill_field(item.first, definition_map, &material_state::output_width);
            fill_field(item.first, definition_map, &material_state::output_height);
            fill_field(item.first, definition_map, &material_state::depth_bias);
            fill_field(item.first, definition_map, &material_state::slope_scaled_depth_bias);
            fill_field(item.first, definition_map, &material_state::stencil_ref);
            fill_field(item.first, definition_map, &material_state::stencil_read_mask);
            fill_field(item.first, definition_map, &material_state::msaa_support);
            fill_field(item.first, definition_map, &material_state::primitive_mode);
            fill_field(item.first, definition_map, &material_state::source_blend_factor);
            fill_field(item.first, definition_map, &material_state::destination_blend_factor);
            fill_field(item.first, definition_map, &material_state::alpha_src);
            fill_field(item.first, definition_map, &material_state::alpha_dst);
            fill_field(item.first, definition_map, &material_state::depth_func);
            fill_field(item.first, definition_map, &material_state::has_transparency);
            fill_field(item.first, definition_map, &material_state::has_cutout);

            LOG(TRACE) << "Filed in all fields on material " << cur_state.name;
        }

        return definitions;
    }

    std::vector<std::pair<material_state, shader_definition>> load_sources_from_folder(const std::string &shaderpack_name, std::vector<std::string>& shader_names) {
        std::vector<shader_definition> sources;

        // Look in the materials directory and load whatever files are there
        // If any of the files have Bedrock names, use the default Bedrock files to make up for anything the user didn't
        //  write
        // If any of the files have Optifine Shaders names, use the default Optifine Shaders files to make up for
        //  anything the user didn't write
        // If we don't find any Optifine Shaders or Bedrock names, the shaderpack is doing its own thing and we should
        //  respect that

        // For right now we're assuming that the material files in the loaded shderpack are the only ones in the world
        // cause it's easier

        std::vector<material_state> materials;

        auto shader_path = std::experimental::filesystem::path("shaderpacks/" + shaderpack_name + "/materials");
        auto directory_iter = std::experimental::filesystem::directory_iterator(shader_path);
        for(const auto& item : directory_iter) {
            // I do like using temporary variables for everything...
            std::stringstream ss;
            ss << item.path();
			auto stringpath = ss.str();

#if __GNUC__
            // std::path's stream insertion operator adds double quotes. yay. I'm so glad the std authors made
            // filesystem so straightforward to use

			stringpath = stringpath.substr(1);
            stringpath = stringpath.substr(0, stringpath.size() - 1);
#endif

            if(!std::experimental::filesystem::is_regular_file(item.path())) {
                LOG(INFO) << "Skipping non-regular file " << stringpath;
                continue;
            }

            if(item.path().extension() != std::experimental::filesystem::path(".material")) {
                LOG(INFO) << "Skipping non-material file " << stringpath;
                continue;
            }

            auto stream = std::ifstream{stringpath};
            auto materials_json = load_json_from_stream(stream);

            auto material_definitions = get_material_definitions(materials_json);
            materials.insert(materials.end(), material_definitions.begin(), material_definitions.end());
        }

        // TODO: fill in missing values from parent states

        auto pack_def = std::vector<std::pair<material_state, shader_definition>>{};

        for(const auto& state : materials) {

            auto shader_def = shader_definition(state);

            bool either_empty = false;

            if(state.vertex_shader) {
                auto vertex_path = "shaderpacks/" + shaderpack_name + "/" + state.vertex_shader.value();
                auto vertex_soruce = load_shader_file(vertex_path, vertex_extensions);
                if (!vertex_soruce.empty()) {
                    shader_def.vertex_source = translate_glsl_to_spirv(vertex_soruce, shaderc_vertex_shader);
                } else {
                    LOG(ERROR) << "No data read for vertex shader " << vertex_path;
                    either_empty = true;
                }
            } else {
                LOG(ERROR) << "Material state " << state.name << " does not define a vertex shader, it will not be loaded";
                continue;
            }

            if(state.fragment_shader) {
                auto fragment_path = "shaderpacks/" + shaderpack_name + "/" + state.fragment_shader.value();
                auto fragment_source = load_shader_file(fragment_path, fragment_extensions);
                if (!fragment_source.empty()) {
                    shader_def.fragment_source = translate_glsl_to_spirv(fragment_source, shaderc_fragment_shader);
                } else {
                    LOG(ERROR) << "No data for fragment shader " << fragment_path;
                    either_empty = true;
                }
            } else {
                LOG(ERROR) << "Material state " << state.name << " does not define a fragment shader, it will not be loaded";
                continue;
            }

            // TODO: Tessellation and geometry

            if (!either_empty) {
                // Missing a vertex of fragment shader? Let's just not load this shader!
                // TODO: Figure out some way to handle a missing essential shader, or make the fallback system super
                // robust
                pack_def.emplace_back(state, shader_def);
            }
        }

        warn_for_missing_fallbacks(sources);

        return pack_def;
    }

    void warn_for_missing_fallbacks(std::vector<shader_definition>& sources) {
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
    auto get_file_path(const std::string &file_path) {
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

    std::string get_filename_from_include(const std::string& include_line) {
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

    std::vector<shader_line> load_shader_file(std::string& shader_path, std::vector<std::string>& extensions) {
        for(auto &extension : extensions) {
            auto full_shader_path = shader_path + extension;
            LOG(TRACE) << "Trying to load shader file " << full_shader_path;

            std::ifstream stream(full_shader_path, std::ios::in);
            if(stream.good()) {
                LOG(INFO) << "Loading shader file " << full_shader_path;
                return read_shader_stream(stream, full_shader_path);
            } else {
                LOG(INFO) << "Could not read file " << full_shader_path;
            }
        }

        LOG(ERROR) << "Could not load shader file " << shader_path;

        return {};
    }

    std::vector<uint32_t> translate_glsl_to_spirv(std::vector<shader_line>& shader_lines, shaderc_shader_kind shader_stage) {

        std::stringstream ss;
        for(auto& line : shader_lines) {
            ss << line.line << "\n";
        }

        // TODO: Cache this
        shaderc::Compiler compiler;
        shaderc::CompileOptions compile_options;
        compile_options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);
        compile_options.SetSourceLanguage(shaderc_source_language_glsl);    // TODO: Auto-detect this
        compile_options.SetWarningsAsErrors();  // TODO: Make this configurable from shaders.json or something
        // TODO: Let users set optimization level too

        auto source = ss.str();
        auto result = compiler.CompileGlslToSpv(source, shader_stage, shader_lines[0].shader_name.c_str(),
                                                        compile_options);

        if(result.GetCompilationStatus() != shaderc_compilation_status_success) {
            LOG(ERROR) << result.GetErrorMessage();
            return {};
        }

        return {result.cbegin(), result.cend()};
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
            auto extensions = std::vector<std::string>{""};
            return load_shader_file(file_to_include, extensions);
        } catch(resource_not_found& e) {
            throw std::runtime_error("Could not load included file " + file_to_include);
        }
    }

    std::vector<std::pair<material_state, shader_definition>> load_sources_from_zip_file(const std::string &shaderpack_name, std::vector<std::string> shader_names) {
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
