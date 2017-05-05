#ifndef VKS_MATERIALTEXTURETYPE
#define VKS_MATERIALTEXTURETYPE

namespace vks {

enum class MatTextureType : uint8_t {
  AMBIENT = 0U,
  DIFFUSE,
  SPECULAR,
  SPECULAR_HIGHLIGHT,
  NORMAL,
  DISPLACEMENT,
  ALPHA,
  size
}; // enum class MatBuilderTextureType

} // namespace vks

#endif
