#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define kProjViewMatricesBindingPos 0

layout (location = 0) in vec3 pos;
layout (location = 0) out vec3 uv_out;

layout (std430, set = 0, binding = kProjViewMatricesBindingPos)
    buffer MainStaticBuffer {
  mat4 proj;
  mat4 view;
  mat4 inv_proj;
  mat4 inv_view;
};

void main() {
  mat4 view_no_pos = mat4(mat3(view));
  vec4 clip_pos = proj * view_no_pos * vec4(pos, 1.f);
  // Use this swizzle so that the depth of the vertex is always 1.0, which is
  // the furthest away
  gl_Position = clip_pos.xyww;

  uv_out = pos;
}
