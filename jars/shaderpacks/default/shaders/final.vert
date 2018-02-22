#version 450

layout(location = 0) in vec3 position_in;
layout(location = 1) in vec2 uv_in;

out vec2 uv;

void main() {
    gl_Position = vec4(position_in, 1);
    uv = uv_in;
}
