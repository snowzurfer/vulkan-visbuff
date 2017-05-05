#version 440

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_shader_draw_parameters : enable

#define kProjViewMatricesBindingPos 0
#define kModelMatricesBindingPos 0

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec3 uv;
layout (location = 3) in vec3 bitangent;
layout (location = 4) in vec3 tangent;

layout (location = 0) flat out uint draw_id;
layout (location = 1) out vec3 norm_vs;
layout (location = 2) out vec3 uv_fs;
layout (location = 3) out vec3 bitangent_vs;
layout (location = 4) out vec3 tangent_vs;

layout (constant_id = 0) const uint num_materials = 1U;
layout (constant_id = 1) const uint num_lights = 1U;

layout (std430, set = 0, binding = kProjViewMatricesBindingPos)
    buffer MainStaticBuffer {
  mat4 proj;
  mat4 view;
  mat4 inv_proj;
  mat4 inv_view;
};


layout (std430, set = 1, binding = kModelMatricesBindingPos) buffer ModelMats {
  mat4 model_mats[];
};

layout(push_constant) uniform PushConsts {
	uint val;
} mesh_id;

void main() {
  mat4 model_view = view * model_mats[mesh_id.val];
  gl_Position = proj * model_view *  vec4(pos, 1.f);

  mat3 transp_model_view = transpose(inverse(mat3(model_view)));
  norm_vs = transp_model_view * norm;

  tangent_vs = transp_model_view * tangent;
  bitangent_vs = transp_model_view * bitangent;

  uv_fs = uv;

  draw_id = mesh_id.val;
}
