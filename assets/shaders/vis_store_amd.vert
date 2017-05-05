#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_shader_draw_parameters : enable

#define kProjViewMatricesBindingPos 0
#define kModelMatricesBindingPos 0

layout (location = 0) in vec3 pos;
layout (location = 2) in vec2 uv_in;

layout (location = 0) flat out uint draw_id;
layout (location = 1) out vec2 uv_out;
layout (location = 2) flat out vec4 pos0;
layout (location = 3)      out vec4 pos1;

layout (constant_id = 0) const uint num_materials = 1U;
layout (constant_id = 1) const uint num_lights = 1U;

struct Light {
  vec4 pos_radius;
  vec4 diff_colour;
  vec4 spec_colour;
};

// Could be packed better but it's kept like this until optimisation stage
struct MatConsts {
  vec4 diffuse_dissolve;
  vec4 specular_shininess;
  vec3 ambient;
  /* 32-bit padding goes here on host side, but GLSL will transform
     the ambient vec3 into a vec4 */
  vec3 emission;
};

layout (std430, set = 0, binding = kProjViewMatricesBindingPos)
    buffer MainStaticBuffer {
  mat4 proj;
  mat4 view;
  mat4 inv_proj;
  mat4 inv_view;
  Light lights[num_lights];
  MatConsts mat_consts[num_materials];
};


layout (std430, set = 1, binding = kModelMatricesBindingPos) buffer ModelMats {
  mat4 model_mats[];
};

layout(push_constant) uniform PushConsts {
	uint val;
} mesh_id;

void main() {
  draw_id = mesh_id.val;
  uv_out = uv_in;
  vec4 temp = proj * view * model_mats[draw_id] * vec4(pos, 1.f);
  pos0 = temp;
  pos1 = temp;
  gl_Position = temp;
}
