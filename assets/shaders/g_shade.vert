#version 440

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define kProjViewMatricesBindingPos 0

layout (location = 0) in vec3 pos;

layout (location = 0) out vec3 view_ray;
layout (location = 1) out vec2 out_uv;

layout (std430, set = 0, binding = kProjViewMatricesBindingPos)
    buffer ProjView {
  mat4 proj;
  mat4 view;
  mat4 inv_proj;
  mat4 inv_view;
};

void main() {
  gl_Position = vec4(pos, 1.f);
	out_uv = pos.xy * 0.5 + 0.5;
  view_ray = (inv_proj * vec4(pos, 1.f)).xyz;
  // Use negative z because reconstructed z is negative in
  // a RH system and would flip XY
  view_ray = vec3(view_ray.xy / (-view_ray.z), -1.f);
}
