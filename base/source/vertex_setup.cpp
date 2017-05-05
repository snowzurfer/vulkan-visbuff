#include <vertex_setup.h>
#include <vulkan/vulkan.h>
#include <vulkan_tools.h>

namespace vks {

VertexElement::VertexElement() : type(), size_bytes(0U), format() {}

VertexElement::VertexElement(VertexElementType Type, uint32_t Size_bytes,
                             VkFormat Format)
    : type(Type), size_bytes(Size_bytes), format(Format) {}

VertexSetup::VertexSetup()
    : vertex_layout_(), vertex_types_layout_(), vertex_size_(0U),
      num_elements_(0U) {}

VertexSetup::VertexSetup(const eastl::vector<VertexElement> &vertex_layout /*,
    const eastl::vector<eastl::pair<VertexElement, uint32_t>> &elements_size*/)
    : vertex_layout_(), vertex_types_layout_(), vertex_size_(0U),
      num_elements_(0U) {
  num_elements_ = SCAST_U32(vertex_layout.size());
  for (uint32_t i = 0U; i < num_elements_; i++) {
    vertex_size_ += vertex_layout[i].size_bytes;

    vertex_layout_[vertex_layout[i].type] = {vertex_layout[i].size_bytes,
                                             vertex_layout[i].format};

    vertex_types_layout_.push_back(vertex_layout[i].type);
  }
}

uint32_t VertexSetup::GetElementSize(uint32_t idx) const {
  return GetElementSize(vertex_types_layout_[idx]);
}

bool VertexSetup::HasElement(VertexElementType element) const {
  return (GetElementSize(element) > 0U) ? true : false;
}

uint32_t VertexSetup::GetElementSize(VertexElementType element) const {
  auto it = vertex_layout_.find(element);
  if (it != vertex_layout_.end()) {
    return it->second.size_bytes;
  }

  ELOG_ERR("Element searched for has not been found!");
  return 0U;
}

VkFormat VertexSetup::GetElementVulkanFormat(uint32_t idx) const {
  return GetElementVulkanFormat(vertex_types_layout_[idx]);
}

VkFormat VertexSetup::GetElementVulkanFormat(VertexElementType element) const {
  auto it = vertex_layout_.find(element);
  if (it != vertex_layout_.end()) {
    return it->second.format;
  }

  ELOG_ERR("Element searched for has not been found!");
  return VK_FORMAT_UNDEFINED;
}

uint32_t VertexSetup::GetElementPosition(uint32_t idx) const {
  return GetElementPosition(vertex_types_layout_[idx]);
}

uint32_t VertexSetup::GetElementPosition(VertexElementType element) const {
  return SCAST_U32(element);
}

} // namespace vks
