//
// Created by David on 26-Apr-16.
//

#ifndef RENDERER_TEXTURE_H
#define RENDERER_TEXTURE_H

#include <exception>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace nova {
    class render_context;

    /*!
     * \brief Encapsulates the parameters for texture filtering
     */
    class texture_filtering_params {
    public:
        enum filter {
            POINT,
            BILINEAR,
            TRILINEAR,
        };

        filter texture_upsample_filter;
        filter texture_downsample_filter;

        int anisotropic_level;
    };

    /*!
     * \brief Represents a two-dimensional OpenGL texture
     */
    class texture2D {
    public:
        texture2D();

        explicit texture2D(std::shared_ptr<render_context> context);

        /*!
         * \brief Sets this texture's data to the given parameters
         *
         * It's worth noting that this function doesn't do any validation on its data. You specified a LDR texture
         * format but you gave me HDR data? Sure hope the GPU can deal with that
         *
         * \param pixel_data The raw pixel_data
         * \param dimensions The dimensions of this texture
         * \param format The format of the texture data
         */
        void set_data(void* pixel_data, vk::Extent2D &dimensions, vk::Format format);

        vk::Extent2D& get_size();

        /*!
         * \brief Returns the format of this texture, as assigned in the constructor
         *
         * \return The format of this texture
         */
        vk::Format& get_format();

        /*!
         * \brief Returns the Vulkan identifier used to identify this texture
         */
        const vk::Image& get_vk_image();

        void set_name(std::string name);
        const std::string& get_name() const;

        /*!
         * \brief Destroys the Vulkan resources associated with this texture
         */
        void destroy();

        vk::ImageView get_image_view();

        vk::ImageLayout get_layout();

    private:
        std::shared_ptr<render_context> context;

        vk::Extent2D size;
        std::string name;

        vk::Image image;
        vk::ImageView image_view;
        vk::Format format;
        vk::ImageLayout layout;
        VmaAllocation allocation;

        void upload_data_with_staging_buffer(void *data, vk::Extent3D image_size);
    };

    /*!
     * \brief Transfers an image from one format to another
     *
     * This function creates a command buffer, submits it, and blocks until the command buffer is finished executing.
     * DO NOT use it on the critical path
     *
     * \param image The image to transition
     * \param format The format of the image
     * \param old_layout The current layout of the image
     * \param new_layout The desired layout of the image
     */
    void transfer_image_format(vk::CommandBuffer command_buffer, vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout, std::shared_ptr<render_context> context);

    void copy_buffer_to_image(vk::CommandBuffer command_buffer, vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, std::shared_ptr<render_context> context);
}

#endif //RENDERER_TEXTURE_H
