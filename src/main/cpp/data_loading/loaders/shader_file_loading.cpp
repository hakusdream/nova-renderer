/*!
 * \brief Implements functions to load a shaderpack from an unzipped shaderpack
 *
 * \author ddubois 
 * \date 22-Feb-18.
 */

#include "shader_loading.h"
#include "../../utils/utils.h"

namespace nova {
    // TODO: Extpand with Bedrock names
    std::vector<std::string> vertex_extensions = {
            ".vert",
            ".vsh"
    };

    // TODO: Expand with Bedrock names
    std::vector<std::string> fragment_extensions = {
            ".frag",
            ".fsh"
    };

    // TODO: Expand with Bedrock names
    std::vector<std::string> geometry_extensions = {
            ".geom",
            ".geo"
    };

    // TODO: Expand with Bedrock names
    std::vector<std::string> tess_eval_extensions = {
            ".frag",
            ".fsh"
    };

    // TODO: Expand with Bedrock names
    std::vector<std::string> tess_control_extensions = {
            ".geom",
            ".geo"
    };

    /*!
     * \brief Tries to load a single shader file from a folder
     *
     * Tries appending each string in extensions to the shader path. If one of the extensions is a real extension
     * of the file, returns the full text of the file. If the file cannot be found with any of the provided
     * extensions, then a not_found is thrown
     *
     * \param shader_path The path to the shader
     * \param extensions A list of extensions to try
     * \return The full source of the shader file
     */
    std::vector<shader_line> load_shader_file(const fs::path &shader_path, const std::vector<std::string> &extensions);

    /*!
     * \brief Loads the shader file from the provided istream
     *
     * \param stream The istream to load the shader file from
     * \param shader_path The path to the shader file (useful mostly for includes)
     * \return A list of shader_line objects
     */
    std::vector<shader_line> read_shader_stream(std::istream &stream, const fs::path &shader_path);

    /*!
     * \brief Loads a file that was requested through a #include statement
     *
     * This function will recursively include files. There's nothing to check for an infinite include loop, so try to
     * not have any
     *
     * \param shader_path The path to the shader that includes the file
     * \param line The line in the shader that contains the #include statement
     * \return The full source of the included file
     */
    std::vector<shader_line> load_included_file(const fs::path &shader_path, const std::string &line);

    /*!
     * \brief Determines the full file path of an included file
     *
     * \param shader_path The path to the current shader
     * \param included_file_name The name of the file to include
     * \return The path to the included file
     */
    fs::path get_included_file_path(const fs::path &shader_path, const fs::path &included_file_name);

    std::unordered_map<std::string, render_pass> load_passes_from_folder(const fs::path &shaderpack_path) {
        auto materials_path = shaderpack_path / "materials";
        if(materials_path.empty()) {
            return {};
        }

        auto passes_map = std::unordered_map<std::string, render_pass>{};
        auto materials_itr = fs::directory_iterator(materials_path);

        for(const auto& item : materials_itr) {
            if(item.path().extension() != ".material") {
                continue;
            }
            // I do like using temporary variables for everything...
            std::stringstream ss;
            ss << item.path();
            // std::path's stream insertion operator adds double quotes. yay. I'm so glad the std authors made
            // filesystem so straightforward to use
            auto stringpath = ss.str().substr(1);
            stringpath = ss.str().substr(0, stringpath.size() - 1);

            auto stream = std::ifstream(stringpath);
            auto materials_json = load_json_from_stream(stream);

            auto material_definitions = parse_passes_from_json(materials_json);
            passes_map.insert(material_definitions.begin(), material_definitions.end());
        }

        return passes_map;
    }

    std::vector<fs::path> get_shader_names_in_folder(const fs::path& shaderpack_path) {
        auto filenames = std::vector<fs::path>{};

        auto files_itr = fs::directory_iterator(shaderpack_path);
        for(const auto& file : files_itr) {
            if(fs::is_regular_file(file.path())) {
                filenames.push_back(file.path().filename());
            }
        }

        return filenames;
    }

    std::unordered_map<std::string, shader_definition> load_sources_from_folder(const fs::path &shaderpack_name, const std::unordered_map<std::string, render_pass> &passes) {
        std::unordered_map<std::string, shader_definition> sources;

        for(const auto& item : passes) {
            const auto& name = item.first;
            const auto& pass_data = item.second;
            auto shader_lines = shader_definition{};

            // The pass data is filled from parent passes, so we should have the fragment shader and vertex shader at
            // least
            if(!pass_data.vertex_shader) {
                LOG(ERROR) << "No vertex shader set for pass " << name << "! Make sure that this pass or one of its parents sets the vertex shader";
            } else {
                shader_lines.vertex_source = load_shader_file(shaderpack_name / pass_data.vertex_shader.value(), vertex_extensions);
            }

            if(!pass_data.fragment_shader) {
                LOG(ERROR) << "No fragment shader set for pass " << name << "! Make sure that this pass or one of its parents sets the fragment shader";
            } else {
                shader_lines.fragment_source = load_shader_file(shaderpack_name / pass_data.fragment_shader.value(), fragment_extensions);
            }

            // Check if we have geometry or tessellation shaders
            if(pass_data.geometry_shader) {
                shader_lines.geometry_source = load_shader_file(shaderpack_name / pass_data.geometry_shader.value(), geometry_extensions);
            }

            if(pass_data.tessellation_control_shader && !pass_data.tessellation_evaluation_shader) {
                LOG(WARNING) << "You've set a tessellation control shader but not an evaluation shader. You need both for this pass to perform tessellation, so Nova will not perform tessellation for this pass";

            } else if(pass_data.tessellation_evaluation_shader && !pass_data.tessellation_control_shader) {
                LOG(WARNING) << "You've set a tessellation evaluation shader but not a control shader. You need both for this pass to perform tessellation, so Nova will not perform tessellation for this pass";

            } else {
                shader_lines.tessellation_evaluation_source = load_shader_file(shaderpack_name / pass_data.tessellation_evaluation_shader.value(), tess_eval_extensions);
                shader_lines.tessellation_control_source = load_shader_file(shaderpack_name / pass_data.tessellation_control_shader.value(), tess_control_extensions);
            }
        }

        return sources;
    }

    std::vector<shader_line> load_shader_file(const fs::path &shader_path, const std::vector<std::string> &extensions) {
        for(auto &extension : extensions) {
            auto full_shader_path = shader_path;
            full_shader_path += extension;
            LOG(TRACE) << "Trying to load shader file " << full_shader_path;

            std::ifstream stream(full_shader_path.string(), std::ios::in);
            if(stream.good()) {
                LOG(INFO) << "Loading shader file " << full_shader_path;
                return read_shader_stream(stream, full_shader_path);
            } else {
                LOG(WARNING) << "Could not read file " << full_shader_path;
            }
        }

        throw resource_not_found(shader_path.string());
    }

    std::vector<shader_line> read_shader_stream(std::istream &stream, const fs::path &shader_path) {
        std::vector<shader_line> file_source;
        std::string line;
        auto line_counter = 1;
        while(std::getline(stream, line, '\n')) {
            if(line.compare("#include") == 0) {
                auto included_file = load_included_file(shader_path, line);
                file_source.insert(file_source.end(), std::begin(included_file), std::end(included_file));

            } else {
                file_source.push_back({line_counter, shader_path, line});
            }

            line_counter++;
        }

        return file_source;
    }

    std::vector<shader_line> load_included_file(const fs::path &shader_path, const std::string &line) {
        auto included_file_name = get_filename_from_include(line);
        auto file_to_include = get_included_file_path(shader_path, included_file_name);
        LOG(TRACE) << "Dealing with included file " << file_to_include;

        try {
            return load_shader_file(file_to_include, {""});
        } catch(resource_not_found& e) {
            throw std::runtime_error("Could not load included file " + file_to_include.string());
        }
    }

    /*!
     * \brief Gets the name of the shaderpack from the file path
     *
     * \param file_path The file path to get the shaderpack name from
     * \return The name of the shaderpack
     */
    auto get_shaderpack_name(const fs::path &file_path) {
        bool should_return_next = false;
        for(const auto& path_part : file_path) {
            if(should_return_next) {
                return path_part;
            }

            should_return_next = true;
        }
    }

    fs::path get_included_file_path(const fs::path &shader_path, const fs::path &included_file_name) {
        if(included_file_name.is_absolute()) {
            // This is an absolute include and it should be relative to the root directory
            auto shaderpack_name = get_shaderpack_name(shader_path);
            return fs::path{"shaderpacks"} / shaderpack_name / "shaders" / included_file_name;

        } else {
            // The include file is a relative include, this one's actually simpler
            return shader_path.parent_path() / included_file_name;
        }
    }
}
