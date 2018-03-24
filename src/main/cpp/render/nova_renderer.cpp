//
// Created by David on 25-Dec-15.
//

#include "nova_renderer.h"
#include "../utils/utils.h"
#include "../data_loading/loaders/loaders.h"
#include "objects/render_object.h"
#include "../data_loading/settings.h"
#include "windowing/glfw_vk_window.h"
#include "vulkan/command_pool.h"
#include "objects/meshes/mesh_store.h"
#include "objects/render_object.h"
#include "objects/meshes/vk_mesh.h"
#include "objects/uniform_buffers/uniform_buffer_definitions.h"
#include "objects/uniform_buffers/uniform_buffer_store.h"
#include "../input/InputHandler.h"
#include "objects/renderpasses/renderpass_manager.h"
#include "vulkan/render_context.h"
#include "objects/shaders/shader_resource_manager.h"

#include <easylogging++.h>
#include <glm/gtc/matrix_transform.hpp>
#include <minitrace.h>

INITIALIZE_EASYLOGGINGPP

#ifdef max
#undef max
#endif

namespace nova {
    std::unique_ptr<nova_renderer> nova_renderer::instance;
    std::shared_ptr<settings> nova_renderer::render_settings;

    nova_renderer::nova_renderer() {
        context = std::make_shared<render_context>();

        game_window = std::make_shared<glfw_vk_window>();
        LOG(TRACE) << "Window initialized";

        context->create_instance(*game_window);
        LOG(TRACE) << "Instance created";
        context->setup_debug_callback();
        LOG(TRACE) << "Debug callback set up";
        game_window->create_surface(context);
        LOG(TRACE) << "Created surface";
        context->find_device_and_queues();
        LOG(TRACE) << "Found device and queue";
        context->create_semaphores();
        LOG(TRACE) << "Created semaphores";
        context->create_command_pool_and_command_buffers();
        LOG(TRACE) << "Created command pool";
        context->create_swapchain(game_window->get_size());
        LOG(TRACE) << "Created swapchain";
        context->create_pipeline_cache();
        LOG(TRACE) << "Pipeline cache created";

        shader_resources = std::make_shared<shader_resource_manager>(context);

        LOG(INFO) << "Vulkan code initialized";

        ubo_manager = std::make_shared<uniform_buffer_store>();
        textures = std::make_shared<texture_manager>(context);
        meshes = std::make_shared<mesh_store>(context, shader_resources);
        inputs = std::make_shared<input_handler>();

		render_settings->register_change_listener(ubo_manager.get());
		render_settings->register_change_listener(game_window.get());
        render_settings->register_change_listener(this);

        render_settings->update_config_loaded();
		render_settings->update_config_changed();

        LOG(DEBUG) << "Finished sending out initial config";

        vk::SemaphoreCreateInfo create_info = {};
        swapchain_image_acquire_semaphore = context->device.createSemaphore(create_info);
        render_finished_semaphore = context->device.createSemaphore(create_info);
        LOG(TRACE) << "Created semaphores";
    }

    nova_renderer::~nova_renderer() {
        // Ensure everything is done before we exit
        context->graphics_queue.waitIdle();
        LOG(TRACE) << "Waited for the GPU to be done";

        inputs.reset();
        LOG(TRACE) << "Released the inputs";
        meshes.reset();
        LOG(TRACE) << "Reset the meshes";
        textures.reset();
        LOG(TRACE) << "Reset the textures";
        ubo_manager.reset();
        LOG(TRACE) << "Reset the UBOs";

        auto& device = context->device;

        device.destroySemaphore(swapchain_image_acquire_semaphore);
        device.destroySemaphore(render_finished_semaphore);
        LOG(TRACE) << "Destroyed the semaphores";

        game_window.reset();
        LOG(TRACE) << "Reset the game window";

        shader_resources.reset();
        LOG(TRACE) << "Reset the shader resource manager";

        mtr_shutdown();
    }

    void nova_renderer::render_frame() {
        begin_frame();

        auto main_command_buffer = context->command_buffer_pool->get_command_buffer(0);

        vk::CommandBufferBeginInfo cmd_buf_begin_info = {};
        main_command_buffer.buffer.begin(cmd_buf_begin_info);
        LOG(TRACE) << "Began command buffer";

        player_camera.recalculate_frustum();

        // Make geometry for any new chunks
        meshes->upload_new_geometry();

        // upload shadow UBO things

        render_shadow_pass();

        update_gbuffer_ubos();

        render_gbuffers(main_command_buffer.buffer);

        render_composite_passes();

        auto window_size = game_window->get_size();

        vk::RenderPassBeginInfo begin_final_pass = vk::RenderPassBeginInfo()
                .setRenderPass(renderpasses->get_final_renderpass())
                .setFramebuffer(renderpasses->get_framebuffer(cur_swapchain_image_index))
                .setRenderArea({{0, 0}, {static_cast<uint32_t>(window_size.x), static_cast<uint32_t>(window_size.y)}});

        main_command_buffer.buffer.beginRenderPass(&begin_final_pass, vk::SubpassContents::eInline);

        render_final_pass();

        // We want to draw the GUI on top of the other things, so we'll render it last
        // Additionally, I could use the stencil buffer to not draw MC underneath the GUI. Could be a fun
        // optimization - I'd have to watch out for when the user hides the GUI, though. I can just re-render the
        // stencil buffer when the GUI screen changes
        render_gui(main_command_buffer.buffer);

        main_command_buffer.buffer.endRenderPass();

        main_command_buffer.buffer.end();

        vk::Semaphore wait_semaphores[] = {swapchain_image_acquire_semaphore};
        vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
        vk::Semaphore signal_semaphores[] = {render_finished_semaphore};

        // TODO: Use the semiphores in render_context
        vk::SubmitInfo submit_info = vk::SubmitInfo()
                .setCommandBufferCount(1)
                .setPCommandBuffers(&main_command_buffer.buffer)
                .setWaitSemaphoreCount(1)
                .setPWaitSemaphores(wait_semaphores)
                .setPWaitDstStageMask(wait_stages)
                .setSignalSemaphoreCount(1)
                .setPSignalSemaphores(signal_semaphores);

        context->graphics_queue.submit(1, &submit_info, vk::Fence());

        cur_swapchain_image_index = context->device.acquireNextImageKHR(context->swapchain, std::numeric_limits<uint32_t>::max(),
                                                                        swapchain_image_acquire_semaphore, vk::Fence()).value;

        vk::Result swapchain_result = {};

        vk::PresentInfoKHR present_info = {};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_finished_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &context->swapchain;
        present_info.pImageIndices = &cur_swapchain_image_index;
        present_info.pResults = &swapchain_result;

        context->present_queue.presentKHR(present_info);

        game_window->end_frame();
    }

    void nova_renderer::render_shadow_pass() {
        LOG(TRACE) << "Rendering shadow pass";
    }

    void nova_renderer::render_gbuffers(vk::CommandBuffer buffer) {
        LOG(TRACE) << "Rendering gbuffer pass";

        // TODO: Get shaders with gbuffers prefix, draw transparents last, etc
        auto& terrain_shader = loaded_shaderpack->get_shader("gbuffers_terrain");
        render_shader(buffer, terrain_shader);
        //auto& water_shader = loaded_shaderpack->get_shader("gbuffers_water");
        //render_shader(buffer, water_shader);
    }

    void nova_renderer::render_composite_passes() {
        LOG(TRACE) << "Rendering composite passes";
    }

    void nova_renderer::render_final_pass() {
        LOG(TRACE) << "Rendering final pass";
        //meshes->get_fullscreen_quad->set_active();
        //meshes->get_fullscreen_quad->draw();
    }

    void nova_renderer::render_gui(vk::CommandBuffer command) {
        LOG(TRACE) << "Rendering GUI";

        update_gui_model_matrices();

        // Bind all the GUI data
        auto &gui_shader = loaded_shaderpack->get_shader("gui");

        command.bindPipeline(vk::PipelineBindPoint::eGraphics, gui_shader.get_pipeline());

        // Render GUI objects
        std::vector<render_object>& gui_geometry = meshes->get_meshes_for_shader("gui");
        for(const auto& geom : gui_geometry) {
            if (!geom.color_texture.empty()) {
                auto color_texture = textures->get_texture(geom.color_texture);
                color_texture.bind(0);
            }

            auto gbuffer_layout = shader_resources->get_layout_for_pass(pass_enum::Gbuffer);
            command.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, gbuffer_layout, 1, 1, &geom.per_model_set, 0, nullptr);

            // Bind the mesh
            vk::DeviceSize offset = 0;
            command.bindIndexBuffer(geom.geometry->indices, offset, vk::IndexType::eUint32);

            command.bindVertexBuffers(0, 1, &geom.geometry->vertex_buffer, &offset);
            offset = 12;
            command.bindVertexBuffers(1, 1, &geom.geometry->vertex_buffer, &offset);
            offset = 20;
            command.bindVertexBuffers(2, 1, &geom.geometry->vertex_buffer, &offset);

            command.drawIndexed(geom.geometry->num_indices, 1, 0, 0, 0);
        }
        LOG(INFO) << "GUI rendering done" << std::endl;
    }

    bool nova_renderer::should_end() {
        // If the window wants to close, the user probably clicked on the "X" button
        return game_window->should_close();
    }

    void nova_renderer::init() {
        mtr_init("nova_profile.json");
        MTR_META_PROCESS_NAME("Nova Renderer")
        MTR_META_THREAD_NAME("Main Nova Thread")

        MTR_SCOPE("INIT", "MainInit")
        render_settings = std::make_shared<settings>("config/config.json");

        try {
            instance = std::make_unique<nova_renderer>();
        } catch(std::exception& e) {
            LOG(ERROR) << "Could not initialize Nova cause " << e.what();
        }
    }

    void nova_renderer::on_config_change(nlohmann::json &new_config) {
		auto& shaderpack_name = new_config["loadedShaderpack"];
		LOG(INFO) << "Shaderpack name: " << shaderpack_name;

        if(!loaded_shaderpack) {
            load_new_shaderpack(shaderpack_name);
            return;
        }

        bool shaderpack_in_settings_is_new = shaderpack_name != loaded_shaderpack->get_name();
        if(shaderpack_in_settings_is_new) {
            load_new_shaderpack(shaderpack_name);
        }
    }

    void nova_renderer::on_config_loaded(nlohmann::json &config) {
        // TODO: Probably want to do some setup here, don't need to do that now
    }

    settings &nova_renderer::get_render_settings() {
        return *render_settings;
    }

    texture_manager &nova_renderer::get_texture_manager() {
        return *textures;
    }

	glfw_vk_window &nova_renderer::get_game_window() {
		return *game_window;
	}

	input_handler &nova_renderer::get_input_handler() {
		return *inputs;
	}

    mesh_store &nova_renderer::get_mesh_store() {
        return *meshes;
    }

    void nova_renderer::load_new_shaderpack(const std::string &new_shaderpack_name) {
		LOG(INFO) << "Loading a new shaderpack named " << new_shaderpack_name;

        auto shader_definitions = load_shaderpack(new_shaderpack_name);

        LOG(DEBUG) << "Shaderpack loaded, wiring everything together";

        vk::Extent2D im_lazy;

        renderpasses = std::make_shared<renderpass_manager>(im_lazy, im_lazy, context->swapchain_extent, context);

        loaded_shaderpack = std::make_shared<shaderpack>(new_shaderpack_name, shader_definitions, renderpasses->get_final_renderpass(), context, shader_resources, game_window->get_size());

        LOG(INFO) << "Loading complete";
		
        link_up_uniform_buffers(loaded_shaderpack->get_loaded_shaders(), ubo_manager);
        LOG(DEBUG) << "Linked up UBOs";
    }

    void nova_renderer::deinit() {
        instance.release();
    }

    void nova_renderer::render_shader(vk::CommandBuffer command, vk_shader_program &shader) {
        LOG(TRACE) << "Rendering everything for shader " << shader.get_name() << " with pipeline " << (VkPipeline)shader.get_pipeline();

        MTR_SCOPE("RenderLoop", "render_shader");
        command.bindPipeline(vk::PipelineBindPoint::eGraphics, shader.get_pipeline());

        auto& geometry = meshes->get_meshes_for_shader(shader.get_name());

        MTR_BEGIN("RenderLoop", "process_all");
        for(auto& geom : geometry) {

            // if(!player_camera.has_object_in_frustum(geom.bounding_box)) {
            //     continue;
            // }

            if(geom.geometry->has_data()) {
                if(!geom.color_texture.empty()) {
                    auto color_texture = textures->get_texture(geom.color_texture);
                    color_texture.bind(0);
                }

                if(geom.normalmap) {
                    textures->get_texture(*geom.normalmap).bind(1);
                }

                if(geom.data_texture) {
                    textures->get_texture(*geom.data_texture).bind(2);
                }

                // upload_model_matrix(geom, shader);

                // Bind the mesh
                vk::DeviceSize offset = 0;
                command.bindVertexBuffers(0, 1, &geom.geometry->vertex_buffer, &offset);
                command.bindIndexBuffer(geom.geometry->indices, offset, vk::IndexType::eUint32);

                command.drawIndexed(geom.geometry->num_indices, 1, 0, 0, 0);
            } else {
                LOG(WARNING) << "Skipping some geometry since it has no data";
            }

        }
        MTR_END("RenderLoop", "process_all")
    }

    void nova_renderer::upload_gui_model_matrix(const render_object& gui_obj, const glm::mat4& model_matrix) {
        // Send the model matrix to the buffer
        // The per-model uniforms buffer is constantly mapped, so we can just grab the mapping from it
        auto& allocation = shader_resources->get_per_model_buffer().get_allocation_info();
        memcpy(((uint8_t*)allocation.pMappedData) + gui_obj.per_model_buffer_range.offset, &model_matrix, gui_obj.per_model_buffer_range.range);
        LOG(INFO) << "Copied the GUI data to the buffer" << std::endl;

        // Copy the memory to the descriptor set
        auto write_ds = vk::WriteDescriptorSet()
            .setDstSet(gui_obj.per_model_set)
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setPBufferInfo((&gui_obj.per_model_buffer_range));

        LOG(INFO) << "Descriptor set: {dstSet=" << write_ds.dstSet << ", dstBinding=" << write_ds.dstBinding
                  << ", dstArrayElement=" << write_ds.dstArrayElement << ", descriptorCount=" << write_ds.descriptorCount
                  << ", descriptorType=" << (int)write_ds.descriptorType << ", pImageInfo=" << write_ds.pImageInfo
                  << ", pBufferInfo=" << write_ds.pBufferInfo << ", pTexelBufferView=" << write_ds.pTexelBufferView
                  << "}";
        context->device.updateDescriptorSets(1, &write_ds, 0, nullptr);
        LOG(INFO) << "Updated the descriptor set" << std::endl;
    }

    void nova_renderer::update_gbuffer_ubos() {
        // Big thing here is to update the camera's matrices

        auto& per_frame_ubo = ubo_manager->get_per_frame_uniforms();

        auto per_frame_uniform_data = per_frame_uniforms{};
        per_frame_uniform_data.gbufferProjection = player_camera.get_projection_matrix();
        per_frame_uniform_data.gbufferModelView = player_camera.get_view_matrix();

        per_frame_ubo.send_data(per_frame_uniform_data);
    }

    camera &nova_renderer::get_player_camera() {
        return player_camera;
    }

    std::shared_ptr<shaderpack> nova_renderer::get_shaders() {
        return loaded_shaderpack;
    }

    void nova_renderer::end_frame() {

    }

    void nova_renderer::begin_frame() {
        LOG(TRACE) << "Beginning frame";
    }

    std::shared_ptr<render_context> nova_renderer::get_render_context() {
        return context;
    }

    std::shared_ptr<shader_resource_manager> nova_renderer::get_shader_resources() {
        return shader_resources;
    }

    void nova_renderer::update_gui_model_matrices() {
        auto& config = render_settings->get_options()["settings"];
        float view_width = config["viewWidth"];
        float view_height = config["viewHeight"];
        float scalefactor = config["scalefactor"];
        // The GUI matrix is super simple, just a viewport transformation
        gui_model = glm::mat4(1.0f);
        gui_model = glm::translate(gui_model, glm::vec3(-1.0f, 1.0f, 0.0f));
        gui_model = glm::scale(gui_model, glm::vec3(scalefactor, scalefactor, 1.0f));
        gui_model = glm::scale(gui_model, glm::vec3(1.0 / view_width, 1.0 / view_height, 1.0));
        gui_model = glm::scale(gui_model, glm::vec3(1.0f, -1.0f, 1.0f));

        LOG(INFO) << "Calculated GUI model matrix for viewWidth=" << view_width << " and viewHeight=" << view_height;

        try {
            if(!meshes) {
                LOG(ERROR) << "Mesh store not initialized! oh no";
                return;
            }

            std::vector<render_object> &gui_objects = meshes->get_meshes_for_shader("gui");
            for(const auto &gui_obj : gui_objects) {
                LOG(INFO) << "Setting the model matrix for a GUI thing";
                upload_gui_model_matrix(gui_obj, gui_model);
            }
        } catch(std::exception& e) {
            LOG(WARNING) << "Load some GUIs you fool";
            LOG(WARNING) << e.what() << std::endl;
        }
    }

    void link_up_uniform_buffers(std::unordered_map<std::string, vk_shader_program> &shaders, std::shared_ptr<uniform_buffer_store> ubos) {
        for(auto& shader : shaders) {
            ubos->register_all_buffers_with_shader(shader.second);
        }
    }
}

