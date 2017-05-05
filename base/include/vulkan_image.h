#ifndef VKS_VULKANIMAGE
#define VKS_VULKANIMAGE

#include <EASTL/vector.h>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace vks {

class VulkanDevice;

enum class CreateView : uint8_t { YES = 0U, NO };

struct VulkanImageInitInfo {
  VkMemoryPropertyFlags memory_properties_flags;
  VkImageCreateInfo create_info;
  CreateView create_view;
  VkImageViewType view_type;
};

struct VulkanImageAcquireInitInfo {
  VkFormat format;
  VkImage image;
  VkImageViewType view_type;
  VkImageUsageFlags image_usages;
  VkExtent3D extents;
  CreateView create_view;
};

class VulkanImage {
public:
  VulkanImage();

  void Init(const VulkanDevice &device, const VulkanImageInitInfo &info);

  void Init(const VulkanDevice &device, const VulkanImageAcquireInitInfo &info);

  void Shutdown(const VulkanDevice &device);

  const VkImage &image() const { return image_; };
  const VkDeviceMemory &memory() const { return memory_; };
  const VkDeviceSize size() const { return size_; };
  const VkImageView view() const { return default_view_; };
  const VkMemoryPropertyFlags &memory_properties_flags() const {
    return memory_properties_flags_;
  }
  const VkImageLayout layout() const { return layout_; };
  uint32_t mip_levels() const { return mip_levels_; };
  const VkExtent3D &extent() const { return extent_; };
  const VkFormat &format() const { return format_; };

  void set_view(VkImageView view) { default_view_ = view; };
  void set_layout(VkImageLayout layout) { layout_ = layout; };

  VkDescriptorImageInfo GetDescriptorImageInfo(VkSampler sampler) const;
  VkDescriptorImageInfo GetDescriptorImageInfo() const;

  VkImageView *CreateAdditionalImageView(
      const VulkanDevice &device,
      const VkImageViewCreateInfo &img_view_create_info) const;

private:
  VkImage image_;
  bool owns_image_;
  VkDeviceMemory memory_;
  VkDeviceSize size_;
  VkImageView default_view_;
  mutable eastl::vector<VkImageView> additional_views_;
  VkMemoryPropertyFlags memory_properties_flags_;
  VkImageLayout layout_;
  VkExtent3D extent_;
  uint32_t mip_levels_;
  uint32_t array_layers_;
  VkFormat format_;

}; // class VulkanImage

} // namespace vks

#endif
