#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_shader_draw_parameters : enable

layout (location = 0) in vec3 uv_in;
layout (location = 0) out vec4 col;

layout (set = 2, binding = 0)
  uniform samplerCube cubemap_texture;

void main() {
  col = texture(cubemap_texture, uv_in);
}
