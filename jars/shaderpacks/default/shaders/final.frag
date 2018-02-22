#version 450

 layout(binding = 0) sampler2D colortex0;

 in vec2 uv;

 out vec3 final_color;

 void main() {
    final_color = texture(colortex0, uv).rgb;
 }