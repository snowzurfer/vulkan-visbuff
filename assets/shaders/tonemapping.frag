#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define kAccumBufferBindingPos 7

layout (set = 0, input_attachment_index = 4, binding = kAccumBufferBindingPos) uniform
  subpassInput accumm_buff;

layout (constant_id = 0) const float kExposure = 0.09f;

layout (location = 0) out vec4 colour;

void main() {
  vec3 hdr_colour = subpassLoad(accumm_buff).rgb;

  // Reinhard tone mapping
  vec3 mapped = vec3(1.f) - exp(-hdr_colour * kExposure);

  colour = vec4(mapped, 1.f);
}