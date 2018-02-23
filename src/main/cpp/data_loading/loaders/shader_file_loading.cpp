/*!
 * \brief Implements functions to load a shaderpack from an unzipped shaderpack
 *
 * \author ddubois 
 * \date 22-Feb-18.
 */

#include "shader_loading.h"
#include "../../utils/utils.h"

namespace nova {
    std::unordered_map<std::string, pass> load_passes_from_folder(const fs::path &shaderpack_path) {
        auto materials_path = shaderpack_path / "materials";
        if(materials_path.empty()) {
            return {};
        }

        auto passes_map = std::unordered_map<std::string, pass>{};
        auto materials_itr = fs::directory_iterator(materials_path);

        for(const auto& item : materials_itr) {
            if(item.path().extension() != ".material") {
                continue;
            }
            // I do like using temporary variables for everything...
            std::stringstream ss;
            ss << item.path();
            // std::path's stream insertino operator adds double quotes. yay. I'm so glad the std authors made
            // filesystem so straightforward to use
            auto stringpath = ss.str().substr(1);
            stringpath = ss.str().substr(0, stringpath.size() - 1);

            auto stream = std::ifstream(stringpath);
            auto materials_json = load_json_from_stream(stream);

            auto material_definitions = get_material_definitions(materials_json);
            passes_map.insert(material_definitions.begin(), material_definitions.end());
        }

        return passes_map;
    }

    std::vector<std::string> get_shader_names_in_folder(const fs::path& shaderpack_path) {
        return {};
    }

    shaderpack load_sources_from_folder(const std::string &shaderpack_name, const std::vector<std::string> &shader_names) {
        std::vector<shader_definition> sources;

        // First, load in the shaders.json file so we can see what we're
        // dealing with
        std::ifstream shaders_json_file("shaderpacks/" + shaderpack_name + "/shaders.json");
        // TODO: Load a default shaders.json file, store it somewhere
        // accessable, and load it if there isn't a shaders.json in the
        // shaderpack
        nlohmann::json shaders_json;
        if(shaders_json_file.is_open()) {
            shaders_json_file >> shaders_json;

        } else {
            shaders_json = get_default_shaders_json();
        }

        // Figure out all the shader files that we need to load
        auto shaders = get_shader_definitions(shaders_json);

        for(auto &shader : shaders) {
            try {
                // All shaderpacks are in the shaderpacks folder
                auto shader_path = "shaderpacks/" + shaderpack_name + "/shaders/" + shader.name;

                shader.vertex_source = load_shader_file(shader_path, {});
                shader.fragment_source = load_shader_file(shader_path, {});

                sources.push_back(shader);
            } catch(std::exception &e) {
                LOG(ERROR) << "Could not load shader " << shader.name << ". Reason: " << e.what();
            }
        }

        warn_for_missing_fallbacks(sources);

        return shaderpack(shaderpack_name, shaders_json, sources);
    }
}
