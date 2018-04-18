/*!
* \brief
*
* \author ddubois
* \date 27-Sep-16.
*/

#include <algorithm>
#include <easylogging++.h>
#include <regex>
#include <iomanip>
#include "mesh_store.h"
#include "../../nova_renderer.h"
#include "vk_mesh.h"

namespace nova {
    mesh_store::mesh_store(std::shared_ptr<render_context> context, std::shared_ptr<shader_resource_manager> shader_resources)
            : context(context), shader_resources(shader_resources) {
        LOG(INFO) << "Does the mesh store have shader resources? " << (bool)shader_resources;
    }

    std::vector<render_object>& mesh_store::get_meshes_for_material(std::string material_name) {
        if(renderables_grouped_by_material.find(material_name) != renderables_grouped_by_material.end()) {
            return renderables_grouped_by_material.at(material_name);
        } else {
            return default_vector;
        }
    }

    void mesh_store::add_gui_buffers(mc_gui_geometry* command) {
        LOG(INFO) << "Adding GUI geometry " << command->texture_name;
        std::string texture_name(command->texture_name);
        texture_name = std::regex_replace(texture_name, std::regex("^textures/"), "");
        texture_name = std::regex_replace(texture_name, std::regex(".png$"), "");
        texture_name = "minecraft:" + texture_name;
        const texture_manager::texture_location tex_location = nova_renderer::instance->get_shader_resources()->get_texture_manager().get_texture_location(texture_name);
        glm::vec2 tex_size = tex_location.max - tex_location.min;

        mesh_definition cur_screen_buffer = {};
        cur_screen_buffer.vertex_data.resize(static_cast<unsigned long>(command->vertex_buffer_size), 0);
        for (int i = 0; i + 8 < command->vertex_buffer_size; i += 9) {
            cur_screen_buffer.vertex_data[i]   = *reinterpret_cast<int*>(&command->vertex_buffer[i]);
            cur_screen_buffer.vertex_data[i+1] = *reinterpret_cast<int*>(&command->vertex_buffer[i+1]);
            cur_screen_buffer.vertex_data[i+2] = *reinterpret_cast<int*>(&command->vertex_buffer[i+2]);
            float u = command->vertex_buffer[i+3] * tex_size.x + tex_location.min.x;
            cur_screen_buffer.vertex_data[i+3] = *reinterpret_cast<int*>(&u);
            float v = command->vertex_buffer[i+4] * tex_size.y + tex_location.min.y;
            cur_screen_buffer.vertex_data[i+4] = *reinterpret_cast<int*>(&v);
            cur_screen_buffer.vertex_data[i+5] = *reinterpret_cast<int*>(&command->vertex_buffer[i+5]);
            cur_screen_buffer.vertex_data[i+6] = *reinterpret_cast<int*>(&command->vertex_buffer[i+6]);
            cur_screen_buffer.vertex_data[i+7] = *reinterpret_cast<int*>(&command->vertex_buffer[i+7]);
            cur_screen_buffer.vertex_data[i+8] = *reinterpret_cast<int*>(&command->vertex_buffer[i+8]);
        }
        cur_screen_buffer.indices.resize(static_cast<unsigned long>(command->index_buffer_size), 0);
        for(int i = 0; i < command->index_buffer_size; i++) {
            cur_screen_buffer.indices[i] = (unsigned int)command->index_buffer[i];
        }

        cur_screen_buffer.vertex_format = format::POS_UV_COLOR;

        render_object gui = {};
        gui.geometry = std::make_unique<vk_mesh>(cur_screen_buffer, context);
        gui.type = geometry_type::gui;

        // The GUI just has a model matrix
        gui.per_model_buffer_range = shader_resources->get_uniform_buffers().get_per_model_buffer()->allocate_space(sizeof(glm::mat4));

        // TODO: Something more intelligent
        if(renderables_grouped_by_material.find("gui") == renderables_grouped_by_material.end()) {
            renderables_grouped_by_material["gui"] = std::vector<render_object>{};
        }
        renderables_grouped_by_material.at("gui").push_back(std::move(gui));
    }

    void mesh_store::remove_gui_render_objects() {
        remove_render_objects([](auto& render_obj) {return render_obj.type == geometry_type::gui;});
    }

    void mesh_store::remove_render_objects(std::function<bool(render_object&)> filter) {
        geometry_to_remove.push(filter);
    }

    void mesh_store::remove_old_geometry() {
        while(!geometry_to_remove.empty()) {
            auto &filter = geometry_to_remove.front();

            auto per_model_buffer = shader_resources->get_uniform_buffers().get_per_model_buffer();
            for (auto &group : renderables_grouped_by_material) {
                auto removed_elements = std::remove_if(group.second.begin(), group.second.end(), filter);

                if (removed_elements != group.second.end()) {
                    // Free the allocations of each render object
                    for (auto it = removed_elements; it != group.second.end(); ++it) {
                        per_model_buffer->free_allocation((*it).per_model_buffer_range);
                    }
                }

                group.second.erase(removed_elements, group.second.end());
            }

            geometry_to_remove.pop();
        }
    }

    void mesh_store::upload_new_geometry() {
        LOG(TRACE) << "Uploading " << chunk_parts_to_upload.size() << " new objects";
        chunk_parts_to_upload_lock.lock();
        while(!chunk_parts_to_upload.empty()) {
            const auto& entry = chunk_parts_to_upload.front();
            const auto& def = std::get<1>(entry);

            render_object obj = {};
            obj.geometry = std::make_unique<vk_mesh>(def, context);
            obj.type = geometry_type::block;
            obj.parent_id = def.id;
            obj.position = def.position;
            obj.bounding_box.center = def.position;
            obj.bounding_box.center.y = 128;
            obj.bounding_box.extents = {16, 128, 16};   // TODO: Make these values come from Minecraft

            const std::string& shader_name = std::get<0>(entry);
            if(renderables_grouped_by_material.find(shader_name) == renderables_grouped_by_material.end()) {
                LOG(INFO) << "Adding a new list of " << shader_name << " objects";
                renderables_grouped_by_material[shader_name] = std::vector<render_object>{};
            }
            renderables_grouped_by_material.at(shader_name).push_back(std::move(obj));

            chunk_parts_to_upload.pop();
        }
        chunk_parts_to_upload_lock.unlock();
    }

    void mesh_store::add_chunk_render_object(std::string filter_name, mc_chunk_render_object &chunk) {
        mesh_definition def = {};
        auto& vertex_data = def.vertex_data;

        for(int i = 0; i < chunk.vertex_buffer_size; i++) {
            vertex_data.push_back(chunk.vertex_data[i]);

            if(i % 7 == 6) {
                // Add 0s for the normals and tangets since we don't compute those yet
                vertex_data.push_back(0);
                vertex_data.push_back(0);
                vertex_data.push_back(0);
                vertex_data.push_back(0);
                vertex_data.push_back(0);
                vertex_data.push_back(0);
            }
        }

        for(int i = 0; i < chunk.index_buffer_size; i++) {
            def.indices.push_back(chunk.indices[i]);
        }

        def.vertex_format = format::all_values()[chunk.format];
        def.position = {chunk.x, chunk.y, chunk.z};
        def.id = chunk.id;

        chunk_parts_to_upload_lock.lock();
        chunk_parts_to_upload.emplace(filter_name, def);
        chunk_parts_to_upload_lock.unlock();
    }

    void mesh_store::remove_render_objects_with_parent(long parent_id) {
        remove_render_objects([&](render_object& obj) { return obj.parent_id == parent_id; });
    }
}
