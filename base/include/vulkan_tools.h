#ifndef VKS_VULKANTOOLS
#define VKS_VULKANTOOLS

#include <cstdint>
#include <logger.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vks {

class VulkanImage;
class VulkanDevice;

namespace tools {

#define STR_EXPAND(str) #str
#define STR(str) STR_EXPAND(str)

#define SCAST_U32(val) static_cast<uint32_t>(val)
#define SCAST_U32PTR(val) static_cast<uint32_t *>(val)
#define SCAST_CU32PTR(val) static_cast<const uint32_t *>(val)
#define SCAST_VOIDPTR(val) static_cast<void *>(val)
#define SCAST_CVOIDPTR(val) static_cast<const void *>(val)
#define SCAST_FLOAT(val) static_cast<float>(val)

// Used to exti the program when something went wrong
// void Exit(const std::string &msg);

#define EXIT(msg)                                                              \
  \
{                                                                         \
    ELOG_ERR(msg);                                                             \
    exit(EXIT_FAILURE);                                                        \
  \
}

std::string VkResultToString(VkResult error_code);
#define VK_CHECK_RESULT(f)                                                     \
  \
{                                                                         \
    VkResult res = (f);                                                        \
    if (res != VK_SUCCESS) {                                                   \
      EXIT(vks::tools::VkResultToString(res));                                 \
    }                                                                          \
  \
}

#ifndef NDEBUG
#define VKS_ASSERT(expr, msg)                                                  \
  \
{                                                                         \
    if (!(expr)) {                                                             \
      EXIT(msg);                                                               \
    }                                                                          \
  \
}
#else
#define VKS_ASSERT(...)
#endif

template <typename E>
constexpr typename std::underlying_type<E>::type ToUnderlying(E e) {
  return static_cast<typename std::underlying_type<E>::type>(e);
}

// template <typename T>
// void VulkanObjectsDeleter(T *obj) {
// T
//}

float Lerp(float a, float b, float t);

bool GetSupportedDepthFormat(VkPhysicalDevice physical_device,
                             VkFormat &depth_format);
bool DoesPhysicalDeviceSupportExtension(
    const char *extension_name,
    const std::vector<VkExtensionProperties> &available_extensions);
// Used by the swapchain creation
uint32_t
GetSwapChainNumImages(const VkSurfaceCapabilitiesKHR &surface_capabilities);
VkSurfaceFormatKHR
GetSwapChainFormat(VkFormat desired_format,
                   const std::vector<VkSurfaceFormatKHR> &surface_formats);
VkExtent2D
GetSwapChainExtent(const VkSurfaceCapabilitiesKHR &surface_capabilities,
                   const uint32_t width, const uint32_t height);
VkImageUsageFlags
GetSwapChainUsageFlags(const VkSurfaceCapabilitiesKHR &surface_capabilities);
VkSurfaceTransformFlagBitsKHR
GetSwapChainTransform(const VkSurfaceCapabilitiesKHR &surface_capabilities);
VkPresentModeKHR
GetSwapChainPresentMode(const std::vector<VkPresentModeKHR> &present_modes);

// Change the layout of an image using a barrier.
// It will be recorded on the command buffer cmd_buff
void SetImageLayout(VkCommandBuffer cmd_buff, VulkanImage &image,
                    VkImageLayout old_image_layout,
                    VkImageLayout new_image_layout,
                    VkImageSubresourceRange subresource_range);
void SetImageLayout(VkCommandBuffer cmd_buff, VulkanImage &image,
                    VkImageLayout old_image_layout,
                    VkImageLayout new_image_layout,
                    VkAccessFlags src_access_flags,
                    VkAccessFlags dst_access_flags,
                    VkImageSubresourceRange subresource_range);

void SetImageLayoutAndExecuteBarrier(const VulkanDevice &device,
                                     VkCommandBuffer cmd_buff,
                                     VulkanImage &image,
                                     VkImageLayout old_image_layout,
                                     VkImageLayout new_image_layout,
                                     VkImageSubresourceRange subresource_range);

void SetImageMemoryBarrier(VkCommandBuffer cmd_buff,
                           const std::vector<VulkanImage *> &images,
                           uint32_t old_family_index, uint32_t new_family_index,
                           VkAccessFlags src_access_mask,
                           VkAccessFlags dst_access_mask,
                           VkImageLayout old_image_layout,
                           VkImageLayout new_image_layout,
                           VkImageSubresourceRange subresource_range,
                           VkPipelineStageFlagBits src_stage_mask,
                           VkPipelineStageFlagBits dst_stage_mask);

void SetImageMemoryBarrier(VkCommandBuffer cmd_buff, VkImage image,
                           uint32_t old_family_index, uint32_t new_family_index,
                           VkAccessFlags src_access_mas,
                           VkAccessFlags dst_access_mask,
                           VkImageLayout old_image_layout,
                           VkImageLayout new_image_layout,
                           VkImageSubresourceRange subresource_range,
                           VkPipelineStageFlagBits src_stage_mask,
                           VkPipelineStageFlagBits dst_stage_mask);

void SetLayoutAndWrite(uint32_t binding, VkDescriptorType descriptor_type,
                       uint32_t descriptor_count,
                       VkShaderStageFlags stage_flags,
                       uint32_t dst_array_element, VkDescriptorSet dst_set,
                       const VkBufferView *pTexelBufferView,
                       const VkDescriptorImageInfo *pImageInfo,
                       const VkDescriptorBufferInfo *pBufferInfo,
                       std::vector<VkDescriptorSetLayoutBinding> &bindings,
                       std::vector<VkWriteDescriptorSet> &writes);

bool DoesFileExist(const std::string &name);

bool Replace(eastl::string &str, const eastl::string &from,
             const eastl::string &to);

// The initialisers are used to avoid typing the structure type and the
// null flags every time a stucture needs to be created
namespace inits {

VkBufferCreateInfo BufferCreateInfo(VkBufferCreateFlags flags = 0U);
VkMemoryAllocateInfo MemoryAllocateInfo();
VkImageCreateInfo
ImageCreateInfo(VkImageCreateFlags flags, VkImageType imageType,
                VkFormat format, VkExtent3D extent, uint32_t mipLevels,
                uint32_t arrayLayers, VkSampleCountFlagBits samples,
                VkImageTiling tiling, VkImageUsageFlags usage,
                VkSharingMode sharingMode, uint32_t queueFamilyIndexCount,
                const uint32_t *pQueueFamilyIndices,
                VkImageLayout initialLayout);
VkImageViewCreateInfo
ImageViewCreateInfo(VkImage image, VkImageViewType viewType, VkFormat format,
                    VkComponentMapping components,
                    VkImageSubresourceRange subresourceRange);
VkCommandBufferBeginInfo
CommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0U);
VkFenceCreateInfo FenceCreateInfo(VkFenceCreateFlags flags = 0x0);
VkSubmitInfo SubmitInfo();
VkSamplerCreateInfo SamplerCreateInfo(
    VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode,
    VkSamplerAddressMode addressModeU, VkSamplerAddressMode addressModeV,
    VkSamplerAddressMode addressModeW, float mipLodBias,
    VkBool32 anisotropyEnable, float maxAnisotropy, VkBool32 compareEnable,
    VkCompareOp compareOp, float minLod, float maxLod,
    VkBorderColor borderColor, VkBool32 unnormalizedCoordinates);
VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo();
VkDeviceQueueCreateInfo DeviceQueueCreateInfo();
VkCommandPoolCreateInfo
CommandPoolCreateInfo(VkCommandPoolCreateFlags flags = 0U);
VkShaderModuleCreateInfo ShaderModuleCreateInfo();
VkComputePipelineCreateInfo ComputePipelineCreateInfo();
VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo();
// VkAttachmentDescription AttachmentDescription();
VkRenderPassCreateInfo RenderPassCreateInfo(
    uint32_t attachmentCount, const VkAttachmentDescription *pAttachments,
    uint32_t subpassCount, const VkSubpassDescription *pSubpasses,
    uint32_t dependencyCount, const VkSubpassDependency *pDependencies);
VkDescriptorSetLayoutCreateInfo DescriptrorSetLayoutCreateInfo();
VkDescriptorPoolCreateInfo
DescriptrorPoolCreateInfo(uint32_t maxSets, uint32_t poolSizeCount,
                          const VkDescriptorPoolSize *pPoolSizes);
VkDescriptorSetAllocateInfo
DescriptorSetAllocateInfo(VkDescriptorPool descriptorPool,
                          uint32_t descriptorSetCount,
                          const VkDescriptorSetLayout *pSetLayouts);
VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                   uint32_t dstArrayElement, uint32_t descriptorCount,
                   VkDescriptorType descriptorType,
                   const VkDescriptorImageInfo *pImageInfo,
                   const VkDescriptorBufferInfo *pBufferInfo = nullptr,
                   const VkBufferView *pTexelBufferView = nullptr);
VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                   uint32_t dstArrayElement, uint32_t descriptorCount,
                   VkDescriptorType descriptorType,
                   const VkBufferView *pTexelBufferView,
                   const VkDescriptorImageInfo *pImageInfo = nullptr,
                   const VkDescriptorBufferInfo *pBufferInfo = nullptr);
VkWriteDescriptorSet
WriteDescriptorSet(VkDescriptorSet dstSet, uint32_t dstBinding,
                   uint32_t dstArrayElement, uint32_t descriptorCount,
                   VkDescriptorType descriptorType,
                   const VkDescriptorBufferInfo *pBufferInfo,
                   const VkBufferView *pTexelBufferView = nullptr,
                   const VkDescriptorImageInfo *pImageInfo = nullptr);
VkPipelineLayoutCreateInfo
PipelineLayoutCreateInfo(uint32_t setLayoutCount,
                         const VkDescriptorSetLayout *pSetLayouts,
                         uint32_t pushConstantRangeCount,
                         const VkPushConstantRange *pPushConstantRanges);
VkFramebufferCreateInfo FramebufferCreateInfo(VkRenderPass renderPass,
                                              uint32_t attachmentCount,
                                              const VkImageView *pAttachments,
                                              uint32_t width, uint32_t height,
                                              uint32_t layers);
VkImageMemoryBarrier ImageMemoryBarrier();
VkImageMemoryBarrier
ImageMemoryBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
                   VkImageLayout oldLayout, VkImageLayout newLayout,
                   uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex,
                   VkImage image, VkImageSubresourceRange subresourceRange);
VkRenderPassBeginInfo RenderPassBeginInfo();
VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo();
VkPipelineViewportStateCreateInfo PipelineViewportStateCreateInfo();
VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo();
VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo();
VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateCreateInfo();
VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateCreateInfo(
    VkBool32 logicOpEnable, VkLogicOp logicOp, uint32_t attachmentCount,
    const VkPipelineColorBlendAttachmentState *pAttachments,
    float *blendConstants);
VkGraphicsPipelineCreateInfo GraphicsPipelineCreateInfo();
VkPresentInfoKHR PresentInfoKHR();
VkAttachmentDescription
AttachmentDescription(VkAttachmentDescriptionFlags flags, VkFormat format,
                      VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp,
                      VkAttachmentStoreOp storeOp,
                      VkAttachmentLoadOp stencilLoadOp,
                      VkAttachmentStoreOp stencilStoreOp,
                      VkImageLayout initialLayout, VkImageLayout finalLayout);
VkSubpassDescription SubpassDescription(
    VkPipelineBindPoint pipelineBindPoint, uint32_t inputAttachmentCount,
    const VkAttachmentReference *pInputAttachments,
    uint32_t colorAttachmentCount,
    const VkAttachmentReference *pColorAttachments,
    const VkAttachmentReference *pResolveAttachments,
    const VkAttachmentReference *pDepthStencilAttachment,
    uint32_t preserveAttachmentCount, const uint32_t *pPreserveAttachments);
VkSubpassDependency SubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass,
                                      VkPipelineStageFlags srcStageMask,
                                      VkPipelineStageFlags dstStageMask,
                                      VkAccessFlags srcAccessMask,
                                      VkAccessFlags dstAccessMask,
                                      VkDependencyFlags dependencyFlags);
VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type,
                                        uint32_t descriptorCount);
VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
    uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers);
VkImageSubresourceRange ImageSubresourceRange(VkImageAspectFlags aspectMask,
                                              uint32_t baseMipLevel,
                                              uint32_t levelCount,
                                              uint32_t baseArrayLayer,
                                              uint32_t layerCount);
VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(
    VkBool32 blendEnable, VkBlendFactor srcColorBlendFactor,
    VkBlendFactor dstColorBlendFactor, VkBlendOp colorBlendOp,
    VkBlendFactor srcAlphaBlendFactor, VkBlendFactor dstAlphaBlendFactor,
    VkBlendOp alphaBlendOp, VkColorComponentFlags colorWriteMask);
VkDescriptorPoolSize DescriptorPoolSize(VkDescriptorType type,
                                        uint32_t descriptorCount);
VkSpecializationMapEntry SpecializationMapEntry(uint32_t constantID,
                                                uint32_t offset, size_t size);
VkStencilOpState StencilOpState(VkStencilOp failOp = VK_STENCIL_OP_KEEP,
                                VkStencilOp passOp = VK_STENCIL_OP_KEEP,
                                VkStencilOp depthFailOp = VK_STENCIL_OP_KEEP,
                                VkCompareOp compareOp = VK_COMPARE_OP_NEVER,
                                uint32_t compareMask = 0x0,
                                uint32_t writeMask = 0x0,
                                uint32_t reference = 0x0);
VkImageBlit ImageBlit(VkImageSubresourceLayers srcSubresource,
                      VkOffset3D srcOffsets[2],
                      VkImageSubresourceLayers dstSubresource,
                      VkOffset3D dstOffsets[2]);

} // namespace inits
} // namespace tools
} // namespace vks

#endif
