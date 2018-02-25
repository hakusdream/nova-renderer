/*!
 * \author ddubois
 * \date 21-Feb-18.
 */

#include "pass.h"
#include <easylogging++.h>
#include "../../utils/utils.h"

namespace nova {
    pass::pass(const std::string& pass_name, const optional<std::string>& parent_pass_name, const nlohmann::json& pass_json) :
        name(pass_name), parent_name(parent_pass_name) {

        states = get_json_value<std::vector<state_enum>>(pass_json, "states", [&](const auto& states) {
            auto vec = std::vector<state_enum>{};
            for(auto& state : states) {
                vec.push_back(state_enum::from_string(state));
            }
            return vec;
        });

        defines = get_json_value<std::vector<std::string>>(pass_json, "defines", [&](const auto& defines) {
            auto vec = std::vector<std::string>{};
            for(auto& define : defines) {
                vec.push_back(define);
            }
            return vec;
        });

        sampler_states = get_json_value<std::vector<sampler_state>>(pass_json, "samplerStates", [&](const auto& sampler_states) {
            auto vec = std::vector<sampler_state>{};
            for(auto& sampler_state : sampler_states) {
                vec.push_back(decode_sampler_state(sampler_state));
            }
            return vec;
        });

        // Sometimes I have to use the type name, sometimes auto works. Go figure
        depth_bias = get_json_value<float>(pass_json, "depthBias");

        slope_scaled_depth_bias = get_json_value<float>(pass_json, "slopeScaledDepthBias");

        depth_bias = get_json_value<float>(pass_json, "depthBiasOGL");

        slope_scaled_depth_bias = get_json_value<float>(pass_json, "slopeScaledDepthBiasOGL");

        vertex_shader = get_json_value<std::string>(pass_json, "vertexShader");

        fragment_shader = get_json_value<std::string>(pass_json, "fragmentShader");

        geometry_shader  = get_json_value<std::string>(pass_json, "geometryShader");

        tessellation_evaluation_shader = get_json_value<std::string>(pass_json, "tessellationEvaluationShader");

        tessellation_control_shader = get_json_value<std::string>(pass_json, "tessellationControlShader");

        vertex_fields = get_json_value<std::vector<vertex_field_enum>>(pass_json, "vertexFields", [&](const nlohmann::json& vertex_fields) {
            auto vec = std::vector<vertex_field_enum>{};
            for(const auto& vertex_field : vertex_fields) {
                vec.push_back(vertex_field_enum::from_string(vertex_field["field"]));
            }
            return vec;
        });

        front_face = get_json_value<stencil_op_state>(pass_json, "frontFace", decode_stencil_buffer_state);

        back_face = get_json_value<stencil_op_state>(pass_json, "backFace", decode_stencil_buffer_state);

        stencil_ref = get_json_value<uint32_t>(pass_json, "stencilRef");

        stencil_read_mask = get_json_value<uint32_t>(pass_json, "stencilReadMask");

        if(stencil_write_mask) {
            if(front_face) {
                (*front_face).compare_mask = stencil_write_mask;
            }
            if(back_face) {
                (*back_face).compare_mask = stencil_write_mask;
            }
        }

        stencil_write_mask = get_json_value<uint32_t>(pass_json, "stencilWriteMask");

        if(stencil_write_mask) {
            if(front_face) {
                (*front_face).write_mask = stencil_write_mask;
            }
            if(back_face) {
                (*back_face).write_mask = stencil_write_mask;
            }
        }

        msaa_support = get_json_value<msaa_support_enum>(pass_json, "msaaSupport", msaa_support_enum::from_string);

        primitive_mode = get_json_value<primitive_topology_enum>(pass_json, "primitiveMode", primitive_topology_enum::from_string);

        source_blend_factor = get_json_value<blend_factor_enum>(pass_json, "blendSrc", blend_factor_enum::from_string);

        destination_blend_factor = get_json_value<blend_factor_enum>(pass_json, "blendDst", blend_factor_enum::from_string);

        textures = get_json_value<std::vector<texture>>(pass_json, "textures", [&](const nlohmann::json& textures) {
            auto vec = std::vector<texture>{};

            for(const auto& texture_field : textures) {
                vec.push_back(decode_texture(texture_field));
            }

            return vec;
        });

        alpha_src = get_json_value<blend_factor_enum>(pass_json, "alphaSrc", blend_factor_enum::from_string);
        alpha_dst = get_json_value<blend_factor_enum>(pass_json, "alphaDst", blend_factor_enum::from_string);
        depth_func = get_json_value<compare_op>(pass_json, "depthFunc", compare_op::from_string);

        filters = get_json_value<std::string>(pass_json, "filters");
        fallback = get_json_value<std::string>(pass_json, "fallback");
        render_queue = get_json_value<render_queue_enum>(pass_json, "renderQueue", render_queue_enum::from_string);

        dependencies = get_json_value<std::vector<std::string>>(pass_json, "dependencies", [&](const nlohmann::json& dependencies) {
            auto vec = std::vector<std::string>{};
            for(const auto& dependency_field : dependencies) {
                vec.push_back(dependency_field);
            }
            return vec;
        });

        texture_inputs = get_json_value<std::vector<texture_resource>>(pass_json, "textureInputs", [&](const nlohmann::json& texture_resources) {
            auto vec = std::vector<texture_resource>{};
            for(const auto& texture_resource_field : texture_resources) {
                vec.push_back(decode_texture_resource(texture_resource_field));
            }
            return vec;
        });
        texture_outputs = get_json_value<std::vector<texture_resource>>(pass_json, "textureOutputs", [&](const nlohmann::json& texture_resources) {
            auto vec = std::vector<texture_resource>{};
            for(const auto& texture_resource_field : texture_resources) {
                vec.push_back(decode_texture_resource(texture_resource_field));
            }
            return vec;
        });
    }

    sampler_state decode_sampler_state(const nlohmann::json& json) {
        sampler_state new_sampler_state = {};

        new_sampler_state.sampler_index = get_json_value<std::uint32_t>(json, "samplerIndex").value_or(0);
        new_sampler_state.filter = get_json_value<texture_filter_enum>(json, "textureFilter", texture_filter_enum::from_string).value_or(texture_filter_enum::Point);
        new_sampler_state.wrap_mode = get_json_value<wrap_mode_enum>(json, "wrapMode", wrap_mode_enum::from_string).value_or(wrap_mode_enum::Clamp);

        return new_sampler_state;
    }

    stencil_op_state decode_stencil_buffer_state(const nlohmann::json &json) {
        auto ret_val = stencil_op_state{};

        ret_val.compare_op = get_json_value<compare_op>(json, "stencilFunc", compare_op::from_string);

        ret_val.fail_op = get_json_value<stencil_op_enum>(json, "stencilFailOp", stencil_op_enum::from_string);

        ret_val.depth_fail_op = get_json_value<stencil_op_enum>(json, "stencilDepthFailOp", stencil_op_enum::from_string);

        ret_val.pass_op = get_json_value<stencil_op_enum>(json, "stencilPassOp", stencil_op_enum::from_string);

        return ret_val;
    }

    texture decode_texture(const nlohmann::json& texture_json) {
        auto tex = texture{};

        // If we don't have these, it's an error
        // TODO: A better parser for this stuff. Maybe extend optional to have an exception or error string or something?
        tex.index = get_json_value<uint32_t>(texture_json, "textureIndex").value();

        tex.texture_location = get_json_value<texture_location_enum>(texture_json, "textureLocation", texture_location_enum::from_string).value();

        tex.texture_name = get_json_value<std::string>(texture_json, "textureName").value();

        tex.calculate_mipmaps = get_json_value<bool>(texture_json, "calculateMipmaps");

        return tex;
    }

    texture_resource decode_texture_resource(const nlohmann::json& json) {
        auto ret_val = texture_resource{};

        // TODO: Check for the implicitly defined textures and use those instead of reading from JSON

        // TODO: The pass should only say the name of the texture it uses. We'll have a separate list of textures that
        // TODO: we read from - which makes using implicitly defined textures trivial for the user

        ret_val.binding = get_json_value<uint32_t>(json, "binding").value();
        ret_val.format = get_json_value<texture_format_enum>(json, "format", texture_format_enum::from_string).value();
        ret_val.height = get_json_value<uint32_t>(json, "height").value();
        ret_val.width = get_json_value<uint32_t>(json, "width").value();
        ret_val.name = get_json_value<std::string>(json, "name").value();

        return ret_val;
    }
}
