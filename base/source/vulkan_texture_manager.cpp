#include <vulkan_device.h>
#include <vulkan_texture_manager.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <EASTL/utility.h>
#include <gli/gli.hpp>
#include <lodepng.h>
#include <logger.hpp>
#include <vulkan_tools.h>

namespace vks {

VulkanTextureManager::VulkanTextureManager()
    : cmd_buffer_(VK_NULL_HANDLE), textures_() {}

void VulkanTextureManager::Init(const VulkanDevice &device) {
  // Create a separate command buffer for submitting
  // image barriers
  VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
      device.graphics_queue().cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1U};

  VK_CHECK_RESULT(vkAllocateCommandBuffers(
      device.device(), &cmd_buffer_allocate_info, &cmd_buffer_));
}

void VulkanTextureManager::Shutdown(const VulkanDevice &device) {
  NameTexMap::iterator iter;
  for (iter = textures_.begin(); iter != textures_.end(); iter++) {
    iter->second->Shutdown(device);
  }

  if (cmd_buffer_ != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(device.device(), device.graphics_queue().cmd_pool, 1U,
                         &cmd_buffer_);
    cmd_buffer_ = VK_NULL_HANDLE;
  }
}

void VulkanTextureManager::Create2DTextureFromData(
    const VulkanDevice &device, const eastl::string &name, const void *data,
    const uint32_t size, const uint32_t width, const uint32_t height,
    VkFormat format, VulkanTexture **texture, const VkSampler sampler,
    const VkImageUsageFlags img_usage_flags) {

  // Setup buffer copy regions for each mip level
  eastl::vector<VkBufferImageCopy> buffer_copy_regions;

  VkBufferImageCopy copy_region;
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.mipLevel = 0U;
  copy_region.imageSubresource.baseArrayLayer = 0U;
  copy_region.imageSubresource.layerCount = 1U;
  copy_region.imageExtent.width = width;
  copy_region.imageExtent.height = height;
  copy_region.imageExtent.depth = 1U;
  copy_region.bufferOffset = 0U;
  copy_region.bufferRowLength = 0U;
  copy_region.bufferImageHeight = 0U;
  copy_region.imageOffset.x = 0U;
  copy_region.imageOffset.y = 0U;
  copy_region.imageOffset.z = 0U;

  buffer_copy_regions.push_back(copy_region);

  CreateTexture(device, name, data, size, width, height, 1U, 1U, format,
                buffer_copy_regions, texture, sampler, img_usage_flags);
}

void VulkanTextureManager::Load2DPNGTexture(
    const VulkanDevice &device, const eastl::string &filename_original,
    VkFormat format, VulkanTexture **texture, const VkSampler aniso_sampler,
    const VkImageUsageFlags img_usage_flags) {
  eastl::string filename(filename_original);
  tools::Replace(filename, "\\", "/");
  tools::Replace(filename, "//", "/");

  // First check if the texture requested is already present
  (*texture) = GetTextureByName(filename);
  if ((*texture) != nullptr) {
    LOG("Texture " + filename + " already exists, returning pre-loaded one.");
    return;
  }

  std::vector<unsigned char> png_data;
  uint32_t width = 0U;
  uint32_t height = 0U;

  uint32_t err = lodepng::decode(png_data, width, height, filename.c_str());

  if (err != 0U) {
    ELOG_WARN("Couldn't find or load texture " + filename + " .");
    (*texture) = nullptr;
    return;
  }

  // Setup buffer copy regions for each mip level
  eastl::vector<VkBufferImageCopy> buffer_copy_regions;

  VkBufferImageCopy copy_region;
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.mipLevel = 0U;
  copy_region.imageSubresource.baseArrayLayer = 0U;
  copy_region.imageSubresource.layerCount = 1U;
  copy_region.imageExtent.width = width;
  copy_region.imageExtent.height = height;
  copy_region.imageExtent.depth = 1U;
  copy_region.bufferOffset = 0U;
  copy_region.bufferRowLength = 0U;
  copy_region.bufferImageHeight = 0U;
  copy_region.imageOffset.x = 0U;
  copy_region.imageOffset.y = 0U;
  copy_region.imageOffset.z = 0U;

  buffer_copy_regions.push_back(copy_region);

  CreateTexture(device, filename, png_data.data(), SCAST_U32(png_data.size()),
                width, height, 1U, 1U, format, buffer_copy_regions, texture,
                aniso_sampler, img_usage_flags);
}

void VulkanTextureManager::CreateTexture(
    const VulkanDevice &device, const eastl::string &name, const void *data,
    const uint32_t size, const uint32_t width, const uint32_t height,
    const uint32_t array_layers, const uint32_t mip_levels, VkFormat format,
    const eastl::vector<VkBufferImageCopy> &copy_regions,
    VulkanTexture **texture, const VkSampler aniso_sampler,
    const VkImageUsageFlags img_usage_flags,
    const VkImageCreateFlags img_create_flags,
    const VkImageViewType img_view_type, const VkImageType img_type) {
  // Create the image
  VkImageCreateInfo image_create_info = tools::inits::ImageCreateInfo(
      img_create_flags, img_type, format, {width, height, 1U}, mip_levels,
      array_layers, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_OPTIMAL,
      img_usage_flags | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE, 0U, nullptr, VK_IMAGE_LAYOUT_UNDEFINED);
  VulkanImageInitInfo image_init_info;
  image_init_info.create_info = image_create_info;
  image_init_info.create_view = CreateView::YES;
  image_init_info.view_type = img_view_type;
  image_init_info.memory_properties_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  eastl::unique_ptr<VulkanImage> image = eastl::make_unique<VulkanImage>();
  image->Init(device, image_init_info);

  if (data != nullptr) {
    VKS_ASSERT(size != 0U, "Size is zero when initial data was passed!");
    // Get the device properties for the requested texture format
    VkFormatProperties format_proerties;
    vkGetPhysicalDeviceFormatProperties(device.physical_device(), format,
                                        &format_proerties);

    // Create a host-visible staging buffer for containing the raw data
    VkBufferCreateInfo buf_create_info = tools::inits::BufferCreateInfo();
    buf_create_info.size = size;
    buf_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buf_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_create_info.queueFamilyIndexCount = 0U;
    buf_create_info.pQueueFamilyIndices = nullptr;

    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateBuffer(device.device(), &buf_create_info, nullptr,
                                   &staging_buffer));

    // Get the memory requirements for the staging buffer
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device.device(), staging_buffer,
                                  &memory_requirements);

    // Allocate memory for the staging buffer
    VkMemoryAllocateInfo mem_alloc_info = tools::inits::MemoryAllocateInfo();
    mem_alloc_info.allocationSize = memory_requirements.size;
    // Retrieve the index for a type of memory visible to the host
    mem_alloc_info.memoryTypeIndex =
        device.GetMemoryType(memory_requirements.memoryTypeBits,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkAllocateMemory(device.device(), &mem_alloc_info, nullptr,
                                     &staging_buffer_memory));

    // Then bind it to the buffer handle
    VK_CHECK_RESULT(vkBindBufferMemory(device.device(), staging_buffer,
                                       staging_buffer_memory, 0U));

    // Now copy the texture data into the staging buffer
    void *mapped_data = nullptr;
    vkMapMemory(device.device(), staging_buffer_memory, 0U,
                memory_requirements.size, 0U, &mapped_data);
    memcpy(mapped_data, data, size);
    vkUnmapMemory(device.device(), staging_buffer_memory);

    // Use an image barrier to setup an optimal image layout for the copy
    VkImageSubresourceRange subresource_range;
    subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel = 0U;
    subresource_range.levelCount = mip_levels;
    subresource_range.baseArrayLayer = 0U;
    subresource_range.layerCount = array_layers;

    // Use the separate command buffer for texture loading
    VkCommandBufferBeginInfo cmd_buff_begin_info =
        tools::inits::CommandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(cmd_buffer_, &cmd_buff_begin_info));

    tools::SetImageLayout(cmd_buffer_, *image.get(), VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          subresource_range);

    // Copy the mip levels from the staging buffer into the image
    vkCmdCopyBufferToImage(cmd_buffer_, staging_buffer, image->image(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           SCAST_U32(copy_regions.size()), copy_regions.data());

    // Change the image layout to shader read so that shaders can sample it
    tools::SetImageLayout(
        cmd_buffer_, *image.get(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresource_range);

    VK_CHECK_RESULT(vkEndCommandBuffer(cmd_buffer_));

    // Use a fence to make sure that the copying operations have completed
    // and then submit the command buffer
    VkFenceCreateInfo fence_create_info = tools::inits::FenceCreateInfo();
    VkFence copy_fence = VK_NULL_HANDLE;
    VK_CHECK_RESULT(vkCreateFence(device.device(), &fence_create_info, nullptr,
                                  &copy_fence));

    VkSubmitInfo submit_info = tools::inits::SubmitInfo();
    submit_info.waitSemaphoreCount = 0U;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1U;
    submit_info.pCommandBuffers = &cmd_buffer_;
    submit_info.signalSemaphoreCount = 0U;
    submit_info.pSignalSemaphores = nullptr;

    vkQueueSubmit(device.graphics_queue().queue, 1U, &submit_info, copy_fence);

    VK_CHECK_RESULT(
        vkWaitForFences(device.device(), 1U, &copy_fence, VK_TRUE, UINT64_MAX));

    // Cleanup the staging resources
    vkDestroyFence(device.device(), copy_fence, nullptr);
    copy_fence = VK_NULL_HANDLE;
    vkFreeMemory(device.device(), staging_buffer_memory, nullptr);
    staging_buffer_memory = VK_NULL_HANDLE;
    vkDestroyBuffer(device.device(), staging_buffer, nullptr);
    staging_buffer = VK_NULL_HANDLE;
  }

  VulkanTextureInitInfo texture_init_info;
  texture_init_info.image = eastl::move(image);
  texture_init_info.create_sampler = CreateSampler::NO;
  texture_init_info.sampler = aniso_sampler;
  texture_init_info.name = name;

  CreateUniqueTexture(device, texture_init_info, name, texture);
}

void VulkanTextureManager::LoadCubeTexture(
    const VulkanDevice &device, const eastl::string &filename_original,
    VulkanTexture **texture, const VkSampler aniso_sampler,
    const VkImageCreateFlags img_create_flags,
    const VkImageUsageFlags img_flags, const VkImageViewType img_view_type) {
  // Change the slashes
  eastl::string filename(filename_original);
  tools::Replace(filename, "\\", "/");
  tools::Replace(filename, "//", "/");

  // First check if the texture requested is already present
  (*texture) = GetTextureByName(filename);
  if ((*texture) != nullptr) {
    LOG("Texture " + filename + " already exists, returning pre-loaded one.");
    return;
  }

  gli::texture_cube tex_cube(gli::load(filename.c_str()));
  if (tex_cube.empty()) {
    ELOG_WARN("Couldn't find or load texture " + filename + " .");
    (*texture) = nullptr;
    return;
  }

  // Get dimensions of first mip level
  uint32_t width = SCAST_U32(tex_cube.extent().x);
  uint32_t height = SCAST_U32(tex_cube.extent().y);
  uint32_t mip_levels = SCAST_U32(tex_cube.levels());

  // Setup buffer copy regions for each mip level
  eastl::vector<VkBufferImageCopy> buffer_copy_regions;
  // Use an offset to go through all the mip levels
  uint32_t offset = 0U;

  for (uint32_t face = 0U; face < 6U; ++face) {
    for (uint32_t i = 0U; i < mip_levels; ++i) {
      VkBufferImageCopy copy_region;
      copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copy_region.imageSubresource.mipLevel = i;
      copy_region.imageSubresource.baseArrayLayer = face;
      copy_region.imageSubresource.layerCount = 1U;
      copy_region.imageExtent.width = SCAST_U32(tex_cube[face][i].extent().x);
      copy_region.imageExtent.height = SCAST_U32(tex_cube[face][i].extent().y);
      copy_region.imageExtent.depth = 1U;
      copy_region.bufferOffset = offset;
      copy_region.bufferRowLength = 0U;
      copy_region.bufferImageHeight = 0U;
      copy_region.imageOffset.x = 0U;
      copy_region.imageOffset.y = 0U;
      copy_region.imageOffset.z = 0U;

      buffer_copy_regions.push_back(copy_region);

      offset += SCAST_U32(tex_cube[face][i].size());
    }
  }

  // Can pass tex_2D.format() because https://github.com/g-truc/gli/issues/85
  CreateTexture(device, filename, tex_cube.data(), SCAST_U32(tex_cube.size()),
                width, height, 6U, mip_levels,
                static_cast<VkFormat>(tex_cube.format()), buffer_copy_regions,
                texture, aniso_sampler, img_flags, img_create_flags,
                img_view_type);
}

void VulkanTextureManager::CreateCubeTextureFromData(
    const VulkanDevice &device, const eastl::string &name, const void *data,
    const uint32_t size, const uint32_t width, const uint32_t height,
    VkFormat format, VulkanTexture **texture, const VkSampler aniso_sampler,
    const VkImageCreateFlags img_create_flags,
    const VkImageUsageFlags img_flags, const VkImageViewType img_view_type) {
  // Setup buffer copy regions for each mip level
  eastl::vector<VkBufferImageCopy> buffer_copy_regions;

  VkBufferImageCopy copy_region;
  copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copy_region.imageSubresource.mipLevel = 0U;
  copy_region.imageSubresource.baseArrayLayer = 0U;
  copy_region.imageSubresource.layerCount = 6U;
  copy_region.imageExtent.width = width;
  copy_region.imageExtent.height = height;
  copy_region.imageExtent.depth = 1U;
  copy_region.bufferOffset = 0U;
  copy_region.bufferRowLength = 0U;
  copy_region.bufferImageHeight = 0U;
  copy_region.imageOffset.x = 0U;
  copy_region.imageOffset.y = 0U;
  copy_region.imageOffset.z = 0U;

  buffer_copy_regions.push_back(copy_region);

  CreateTexture(device, name, data, size, width, height, 6U, 1U, format,
                buffer_copy_regions, texture, aniso_sampler, img_flags,
                img_create_flags, img_view_type);
}

void VulkanTextureManager::Load2DTexture(
    const VulkanDevice &device, const eastl::string &filename_original,
    VulkanTexture **texture, const VkSampler aniso_sampler,
    const VkImageUsageFlags img_usage_flags) {
  // Change the slashes
  eastl::string filename(filename_original);
  tools::Replace(filename, "\\", "/");
  tools::Replace(filename, "//", "/");

  // First check if the texture requested is already present
  (*texture) = GetTextureByName(filename);
  if ((*texture) != nullptr) {
    LOG("Texture " + filename + " already exists, returning pre-loaded one.");
    return;
  }

  gli::texture2d tex_2D(gli::load(filename.c_str()));
  if (tex_2D.empty()) {
    ELOG_WARN("Couldn't find or load texture " + filename + " .");
    (*texture) = nullptr;
    return;
  }

  // Get dimensions of first mip level
  uint32_t width = SCAST_U32(tex_2D[0U].extent().x);
  uint32_t height = SCAST_U32(tex_2D[0U].extent().y);
  uint32_t mip_levels = SCAST_U32(tex_2D.levels());

  // Setup buffer copy regions for each mip level
  eastl::vector<VkBufferImageCopy> buffer_copy_regions;
  // Use an offset to go through all the mip levels
  uint32_t offset = 0U;

  for (uint32_t i = 0U; i < mip_levels; ++i) {
    VkBufferImageCopy copy_region;
    copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.imageSubresource.mipLevel = i;
    copy_region.imageSubresource.baseArrayLayer = 0U;
    copy_region.imageSubresource.layerCount = 1U;
    copy_region.imageExtent.width = SCAST_U32(tex_2D[i].extent().x);
    copy_region.imageExtent.height = SCAST_U32(tex_2D[i].extent().y);
    copy_region.imageExtent.depth = 1U;
    copy_region.bufferOffset = offset;
    copy_region.bufferRowLength = 0U;
    copy_region.bufferImageHeight = 0U;
    copy_region.imageOffset.x = 0U;
    copy_region.imageOffset.y = 0U;
    copy_region.imageOffset.z = 0U;

    buffer_copy_regions.push_back(copy_region);

    offset += static_cast<uint32_t>(tex_2D[i].size());
  }

  // Can pass tex_2D.format() because https://github.com/g-truc/gli/issues/85
  CreateTexture(device, filename, tex_2D.data(), SCAST_U32(tex_2D.size()),
                width, height, 1U, mip_levels,
                static_cast<VkFormat>(tex_2D.format()), buffer_copy_regions,
                texture, aniso_sampler, img_usage_flags);
}

VulkanTexture *
VulkanTextureManager::GetTextureByName(const eastl::string &name) {
  NameTexMap::iterator iter = textures_.find(name);
  if (iter != textures_.end()) {
    return iter->second.get();
  } else {
    return nullptr;
  }
}

void VulkanTextureManager::CreateUniqueTexture(const VulkanDevice &device,
                                               VulkanTextureInitInfo &init_info,
                                               const eastl::string &name,
                                               VulkanTexture **texture) {
  textures_[name] = eastl::make_unique<VulkanTexture>();
  textures_[name]->Init(device, init_info);
  (*texture) = textures_[name].get();
}

} // namespace vks
