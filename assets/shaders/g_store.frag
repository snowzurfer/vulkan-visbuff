#version 440

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_ARB_shader_image_load_store : enable

#define kProjViewMatricesBindingPos 0
#define kModelMatricesBindingPos 0
#define kLightsArrayBindingPos 8
#define kMatConstsArrayBindingPos 11
#define kIndirectDrawCmdsBindingPos 3
#define kVertexBufferBindingPos 4
#define kIndexBufferBindingPos 2
#define kMaterialIDsBindingPos 1
#define kDiffuseTexturesArrayBindingPos 2
#define kAmbientTexturesArrayBindingPos 3
#define kSpecularTexturesArrayBindingPos 4
#define kNormalTexturesArrayBindingPos 5
#define kRoughnessTexturesArrayBindingPos 6

layout (location = 0) flat in uint draw_id;
layout (location = 1) in vec3 norm_vs;
layout (location = 2) in vec3 uv_fs;
layout (location = 3) in vec3 bitangent_vs;
layout (location = 4) in vec3 tangent_vs;

layout (location = 0) out vec4 diffuse_albedo;
layout (location = 1) out vec4 specular_albedo;
layout (location = 2) out vec4 normal_vs;

layout(early_fragment_tests) in;

struct VkDrawIndexedIndirectCommand {
  uint indexCount;
  uint instanceCount;
  uint firstIndex;
  int vertexOffset;
  uint firstInstance;
};

struct Light {
  vec4 pos_radius;
  vec4 diff_colour;
  vec4 spec_colour;
};

// Could be packed better but it's kept like this until optimisation stage
struct MatConsts {
  vec4 diffuse_dissolve;
  vec4 specular_shininess;
  vec4 ambient;
  /* 32-bit padding goes here on host side, but GLSL will transform
     the ambient vec3 into a vec4 */
  vec4 emission;
};

layout (constant_id = 0) const uint num_materials = 1U;
layout (constant_id = 1) const uint num_lights = 1U;

layout (std430, set = 0, binding = kProjViewMatricesBindingPos)
    buffer MainStaticBuffer {
  mat4 proj;
  mat4 view;
  mat4 inv_proj;
  mat4 inv_view;
};


layout (std430, set = 0, binding = kMatConstsArrayBindingPos)
    buffer MatConstsArray {
  MatConsts mat_consts[num_materials];
};

layout (set = 0, binding = kDiffuseTexturesArrayBindingPos)
  uniform sampler2D[num_materials] diff_textures;
layout (set = 0, binding = kAmbientTexturesArrayBindingPos)
  uniform sampler2D[num_materials] amb_textures;
layout (set = 0, binding = kSpecularTexturesArrayBindingPos)
  uniform sampler2D[num_materials] spec_textures;
layout (set = 0, binding = kNormalTexturesArrayBindingPos)
  uniform sampler2D[num_materials] norm_textures;
layout (set = 0, binding = kRoughnessTexturesArrayBindingPos)
  uniform sampler2D[num_materials] rough_textures;

layout (std430, set = 1, binding = kMaterialIDsBindingPos) buffer MatIDs {
  uint mat_ids[];
};

void main() {
  uint mat_id = mat_ids[draw_id];
  diffuse_albedo.rgb = 
    texture(diff_textures[mat_id], uv_fs.xy).rgb * mat_consts[mat_id].diffuse_dissolve.rgb;
  diffuse_albedo.a = 1.f;

  mat3 tangent_frame_vs = mat3(
    normalize(tangent_vs),
    normalize(bitangent_vs),
    normalize(norm_vs));

  /* Sample the tangent space normal map */
  vec3 normal_ts = texture(norm_textures[mat_id], uv_fs.xy).rgb;
  normal_ts = normalize((normal_ts * 2.f) - 1.f);

  normal_vs = vec4(tangent_frame_vs * normal_ts, 1.f);
  normal_vs.w = texture(rough_textures[mat_id], uv_fs.xy).r;
  normal_vs.w = normal_vs.w * mat_consts[mat_id].specular_shininess.a;

  specular_albedo = vec4(
    texture(spec_textures[mat_id], uv_fs.xy).rgb, 1.f);
  specular_albedo.rgb = specular_albedo.rgb * mat_consts[mat_id].specular_shininess.rgb;

  vec3 ambient_albedo =
    texture(amb_textures[mat_id], uv_fs.xy).rgb *
      mat_consts[mat_id].ambient.rgb;
}
