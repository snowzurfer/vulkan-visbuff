#ifndef VKS_VERTEXSETUP
#define VKS_VERTEXSETUP

#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <cstdint>
#include <unordered_map>
#include <vulkan/vulkan.h>

namespace vks {

enum class VertexElementType : uint8_t {
  POSITION = 0U,
  NORMAL,
  UV,
  TANGENT,
  BITANGENT,
  COLOUR,
  num_items
}; // enum class VertexElementType

struct VertexElementTypeHash {
  template <typename T> std::size_t operator()(T t) const {
    return static_cast<std::size_t>(t);
  }
}; // struct VertexElementTypeHash

struct VertexElement {
  VertexElement();
  VertexElement(VertexElementType Type, uint32_t Size_bytes, VkFormat Format);

  VertexElementType type;
  uint32_t size_bytes;
  VkFormat format;
}; // struct VertexElement

class VertexSetup {
public:
	VertexSetup();
  VertexSetup(const eastl::vector<VertexElement> &vertex_layout /*,
      const eastl::vector<eastl::pair<VertexElement, uint32_t>> &elements_size*/);

  const eastl::vector<VertexElementType> &vertex_types_layout() const {
    return vertex_types_layout_;
  }

  uint32_t vertex_size() const { return vertex_size_; }
  uint32_t num_elements() const { return num_elements_; }

  VkFormat GetElementVulkanFormat(uint32_t idx) const;
  VkFormat GetElementVulkanFormat(VertexElementType element) const;

  uint32_t GetElementPosition(uint32_t idx) const;
  uint32_t GetElementPosition(VertexElementType element) const;

  uint32_t GetElementSize(uint32_t idx) const;
  uint32_t GetElementSize(VertexElementType element) const;

  bool HasElement(VertexElementType element) const;

private:
  struct LayoutElementData {
    uint32_t size_bytes;
    VkFormat format;
  };
  std::unordered_map<VertexElementType, LayoutElementData,
                     VertexElementTypeHash>
      vertex_layout_;
  eastl::vector<VertexElementType> vertex_types_layout_;

  uint32_t vertex_size_;
  uint32_t num_elements_;
}; // class VertexSetup

} // namespace vks

#endif
