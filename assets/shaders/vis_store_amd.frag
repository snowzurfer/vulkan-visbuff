#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_AMD_shader_explicit_vertex_parameter : enable
#extension GL_ARB_shader_image_load_store : enable

layout(early_fragment_tests) in;

layout (location = 0) flat in uint draw_id;
layout (location = 1) in vec2 uv_in;
layout (location = 2) flat in vec4 pos0;
layout (location = 3) __explicitInterpAMD in vec4 pos1;

// Y = I,J bary coords, K can be calculated
layout (location = 0) out uvec2 id_and_barys;
// X,Y = dFdX, dFdY
layout (location = 1) out uvec2 derivs;
layout (location = 2) out vec4 debug_out;

uint calculate_output_VBID(bool opaque, uint draw_id, uint primitive_id) {
  uint drawID_primID = ((draw_id << 23) & 0x7F800000) |
                       (primitive_id & 0x007FFFFF);
  if (opaque) {
    return drawID_primID;
  }
  else {
    return (1 << 31) | drawID_primID;
 }
}

void main() {
  id_and_barys.x = calculate_output_VBID(true, draw_id, gl_PrimitiveID);
  vec4 v0 = interpolateAtVertexAMD(pos1, 0);
  vec4 v1 = interpolateAtVertexAMD(pos1, 1);
  vec4 v2 = interpolateAtVertexAMD(pos1, 2);
  if (v0 == pos0) {
    debug_out.y = gl_BaryCoordSmoothAMD.x;
    debug_out.z = gl_BaryCoordSmoothAMD.y;
    debug_out.x = 1 - debug_out.z - debug_out.y;
  }
  else if (v1 == pos0) {
    debug_out.x = gl_BaryCoordSmoothAMD.x;
    debug_out.y = gl_BaryCoordSmoothAMD.y;
    debug_out.z = 1 - debug_out.x - debug_out.y;
  } else if (v2 == pos0) {
    debug_out.z = gl_BaryCoordSmoothAMD.x;
    debug_out.x = gl_BaryCoordSmoothAMD.y;
    debug_out.y = 1 - debug_out.x - debug_out.z;
  }

  derivs.x = packSnorm2x16(dFdx(uv_in));
  derivs.y = packSnorm2x16(dFdy(uv_in));
  id_and_barys.y = packUnorm2x16(debug_out.xy);

	debug_out.z = 0;
}
