#include <array>
#include <base_system.h>
#include <camera.h>
#include <cassert>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.inl>
#include <glm/mat4x4.hpp>
#include <logger.hpp>
#include <material_constants.h>
#include <meshes_heap_manager.h>
#include <model.h>
#include <renderer.h>
#include <vertex_setup.h>
#include <vulkan_device.h>
#include <vulkan_tools.h>

namespace vks {

const VkFormat kVisBarysBufferFormat = VK_FORMAT_R32G32_UINT;
const VkFormat kDerivsBufferFormat = VK_FORMAT_R32G32_UINT;
extern const VkFormat kColourBufferFormat = VK_FORMAT_B8G8R8A8_SRGB;
extern const uint32_t kMapsBaseBindingPos = 0U;
const VkFormat kAccumulationFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
const uint32_t kSpecInfoDrawCmdsCountID = 0U;
const uint32_t kUniformBufferDescCount = 5U;
const uint32_t kMainStaticBuffBindingPos = 0U;
const uint32_t kLightsArrayBindingPos = 10U;
const uint32_t kMatConstsArrayBindingPos = 9U;
const uint32_t kDepthBuffBindingPos = 1U;
const uint32_t kDiffuseTexturesArrayBindingPos = 2U;
const uint32_t kAmbientTexturesArrayBindingPos = 3U;
const uint32_t kSpecularTexturesArrayBindingPos = 4U;
const uint32_t kNormalTexturesArrayBindingPos = 5U;
const uint32_t kRoughnessTexturesArrayBindingPos = 6U;
const uint32_t kVisBufferBindingPos = 7U;
const uint32_t kAccumulationBufferBindingPos = 8U;
const uint32_t kDerivsBarysBufferBindingPos = 11U;
const uint32_t kPerfCounterBufferBindingPos = 12U;
const uint32_t kSkyboxTextureBindingPos = 0U;
const uint32_t kMaxNumUniformBuffers = 5U;
const uint32_t kMaxNumSSBOs = 1000U;
const uint32_t kMaxNumMatInstances = 1000U;
const uint32_t kMaxNumInputAttachments = 5U;
const uint32_t kNumMaterialsSpecConstPos = 0U;
const uint32_t kNumLightsSpecConstPos = 1U;
const uint32_t kTonemapExposureSpecConstPos = 0U;
const float kTonemapExposure = 0.02f;
extern const uint32_t kVertexBuffersBaseBindPos;
extern const uint32_t kIndirectDrawCmdsBindingPos;
extern const uint32_t kIdxBufferBindPos;
extern const uint32_t kModelMatxsBufferBindPos;
extern const uint32_t kMaterialIDsBufferBindPos;
extern const int32_t kWindowWidth;
extern const int32_t kWindowHeight;
const eastl::string kBaseShaderAssetsPath = STR(ASSETS_FOLDER) "shaders/";

Renderer::Renderer()
    : renderpass_(), framebuffers_(), current_swapchain_img_(0U),
      cmd_buffers_(), vis_buffer_(), depth_buffer_(), vis_shade_material_(),
      vis_store_material_(), tonemap_material_(), skybox_material_(),
      dummy_texture_(), desc_set_layouts_(VK_NULL_HANDLE), desc_sets_(),
      desc_pool_(VK_NULL_HANDLE), pipe_layouts_(VK_NULL_HANDLE),
      main_static_buff_(), proj_mat_(1.f), view_mat_(1.f), inv_proj_mat_(1.f),
      inv_view_mat_(1.f), cam_(nullptr), aniso_sampler_(VK_NULL_HANDLE),
      nearest_sampler_(VK_NULL_HANDLE), aniso_edge_sampler_(VK_NULL_HANDLE),
      registered_models_(), fullscreenquad_(nullptr), cube_(nullptr),
      mat_consts_(), renderpasses_fence_(VK_NULL_HANDLE), frames_captured_(0U),
      num_captures_(0U), num_captures_to_collect_(0U),
      capturing_from_positions_enabled_(false), capturing_enabled_(false),
      mem_perf_data_reads_(), mem_perf_data_writes_(),
      camera_sample_positions_(), camera_sample_directions_(),
      capture_screenshot_(false), first_run_(true), vtx_setup_() {}

void Renderer::Init(szt::Camera *cam, const VertexSetup &vtx_setup) {
  cam_ = cam;
  vtx_setup_ = vtx_setup;

  SetupSamplers(vulkan()->device());
  SetupDescriptorPool(vulkan()->device());

  model_manager()->set_aniso_sampler(aniso_sampler_);
  model_manager()->set_shade_material_name("vis_store");
  model_manager()->set_sets_desc_pool(desc_pool_);
  meshes_heap_manager()->set_aniso_sampler(aniso_sampler_);
  meshes_heap_manager()->set_shade_material_name("vis_store");
  meshes_heap_manager()->set_heap_sets_desc_pool(desc_pool_);

  texture_manager()->Load2DTexture(vulkan()->device(),
                                   STR(ASSETS_FOLDER) "dummy.ktx",
                                   &dummy_texture_, aniso_sampler_);
  texture_manager()->LoadCubeTexture(vulkan()->device(),
                                     STR(ASSETS_FOLDER) "skybox.dds",
                                     &skybox_texture_, aniso_edge_sampler_);

  UpdatePVMatrices();
  SetupMaterials();
  SetupRenderPass(vulkan()->device());
  SetupFrameBuffers(vulkan()->device());
  CreateFences(vulkan()->device());
  SetupFullscreenQuad(vulkan()->device());
  CreateCubeMesh(vulkan()->device());
}

void Renderer::FinalInit(const VulkanDevice &device) {
  SetupDescriptorSetAndPipeLayout(device);
  for (eastl::vector<Model *>::iterator itor = registered_models_.begin();
       itor != registered_models_.end(); ++itor) {
    (*itor)->CreateAndWriteDescriptorSets(
        vulkan()->device(), desc_set_layouts_[DescSetLayoutTypes::HEAP]);
  }
  SetupUniformBuffers(device);
  SetupMaterialPipelines(device, vtx_setup_);
  SetupDescriptorSets(device);
  SetupCommandBuffers();
}

void Renderer::CreateFences(const VulkanDevice &device) {
  VkFenceCreateInfo fence_create_info = tools::inits::FenceCreateInfo();

  VK_CHECK_RESULT(vkCreateFence(device.device(), &fence_create_info, nullptr,
                                &renderpasses_fence_));
}

void Renderer::SetupSamplers(const VulkanDevice &device) {
  // Create an aniso sampler
  VkSamplerCreateInfo sampler_create_info = tools::inits::SamplerCreateInfo(
      VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT,
      VK_SAMPLER_ADDRESS_MODE_REPEAT, 0.f, VK_TRUE,
      device.physical_properties().limits.maxSamplerAnisotropy, VK_FALSE,
      VK_COMPARE_OP_NEVER, 0.f, 11.f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      VK_FALSE);

  VK_CHECK_RESULT(vkCreateSampler(device.device(), &sampler_create_info,
                                  nullptr, &aniso_sampler_));

  // Create a nearest neighbour sampler
  sampler_create_info = tools::inits::SamplerCreateInfo(
      VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.f, VK_FALSE, 0U, VK_FALSE,
      VK_COMPARE_OP_NEVER, 0.f, 1.f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      VK_FALSE);

  VK_CHECK_RESULT(vkCreateSampler(device.device(), &sampler_create_info,
                                  nullptr, &nearest_sampler_));

  // Create an aniso sampler with clamp to edge
  sampler_create_info = tools::inits::SamplerCreateInfo(
      VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 0.f, VK_TRUE,
      device.physical_properties().limits.maxSamplerAnisotropy, VK_FALSE,
      VK_COMPARE_OP_NEVER, 0.f, 11.f, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
      VK_FALSE);

  VK_CHECK_RESULT(vkCreateSampler(device.device(), &sampler_create_info,
                                  nullptr, &aniso_edge_sampler_));
}

void Renderer::Shutdown() {
  vkDeviceWaitIdle(vulkan()->device().device());

  renderpass_.reset(nullptr);
  framebuffers_.clear();

  if (desc_pool_ != VK_NULL_HANDLE) {
    VK_CHECK_RESULT(
        vkResetDescriptorPool(vulkan()->device().device(), desc_pool_, 0U));
    vkDestroyDescriptorPool(vulkan()->device().device(), desc_pool_, 0U);
    desc_pool_ = VK_NULL_HANDLE;
  }

  for (uint32_t i = 0U; i < PipeLayoutTypes::num_items; i++) {
    vkDestroyPipelineLayout(vulkan()->device().device(), pipe_layouts_[i],
                            nullptr);
  }
  pipe_layouts_.clear();

  for (uint32_t i = 0U; i < DescSetLayoutTypes::num_items; i++) {
    vkDestroyDescriptorSetLayout(vulkan()->device().device(),
                                 desc_set_layouts_[i], nullptr);
  }
  desc_set_layouts_.clear();

  if (aniso_sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(vulkan()->device().device(), aniso_sampler_, nullptr);
    aniso_sampler_ = VK_NULL_HANDLE;
  }
  if (nearest_sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(vulkan()->device().device(), nearest_sampler_, nullptr);
    nearest_sampler_ = VK_NULL_HANDLE;
  }
  if (aniso_edge_sampler_ != VK_NULL_HANDLE) {
    vkDestroySampler(vulkan()->device().device(), aniso_edge_sampler_, nullptr);
    aniso_edge_sampler_ = VK_NULL_HANDLE;
  }

  // tonemap_material_.Shutdown(vulkan()->device());
  // vis_shade_material_.Shutdown(vulkan()->device());
  // vis_store_material_.Shutdown(vulkan()->device());

  main_static_buff_.Shutdown(vulkan()->device());

  vkDestroyFence(vulkan()->device().device(), renderpasses_fence_, nullptr);

  OutputPerformanceDataToFile();
}

void Renderer::PreRender() {
  if (first_run_) {
    FinalInit(vulkan()->device());
    first_run_ = false;
  }

  UpdateBuffers(vulkan()->device());

  vulkan()->swapchain().AcquireNextImage(vulkan()->device(),
                                         vulkan()->image_available_semaphore(),
                                         current_swapchain_img_);
}

void Renderer::UpdateBuffers(const VulkanDevice &device) {
  UpdatePVMatrices();

  if (capturing_from_positions_enabled_) {
    // Position camera at pre-defined positions
    view_mat_ = glm::lookAt(camera_sample_positions_[num_captures_],
                            camera_sample_directions_[num_captures_] +
                                camera_sample_positions_[num_captures_],
                            glm::vec3(0.f, 1.f, 0.f));
  }

  eastl::vector<Light> transformed_lights;
  UpdateLights(transformed_lights);

  // Cache some sizes
  uint32_t num_mat_instances = material_manager()->GetMaterialInstancesCount();
  uint32_t num_lights = SCAST_U32(transformed_lights.size());
  uint32_t mat4_size = SCAST_U32(sizeof(glm::mat4));
  uint32_t mat4_group_size = mat4_size * 4U;
  uint32_t lights_array_size = (SCAST_U32(sizeof(Light)) * num_lights);
  uint32_t mat_consts_array_size =
      (SCAST_U32(sizeof(MaterialConstants)) * num_mat_instances);

  // Upload data to the buffers
  eastl::array<glm::mat4, 4U> matxs_initial_data = {
      proj_mat_, view_mat_, inv_proj_mat_, inv_view_mat_};

  void *mapped = nullptr;
  main_static_buff_.Map(device, &mapped);
  uint8_t *mapped_u8 = static_cast<uint8_t *>(mapped);

  memcpy(mapped, matxs_initial_data.data(), mat4_group_size);
  mapped_u8 += mat4_group_size;

  memcpy(mapped_u8, transformed_lights.data(), lights_array_size);
  mapped_u8 += lights_array_size;

  memcpy(mapped_u8, mat_consts_.data(), mat_consts_array_size);
  mapped_u8 += mat_consts_array_size;

  uint32_t zero_val = 0U;
  memcpy(mapped_u8, &zero_val, sizeof(zero_val));
  memcpy(mapped_u8 + sizeof(zero_val), &zero_val, sizeof(zero_val));
  memcpy(mapped_u8 + sizeof(zero_val) * 2, &zero_val, sizeof(zero_val));
  memcpy(mapped_u8 + sizeof(zero_val) * 3, &zero_val, sizeof(zero_val));

  main_static_buff_.Unmap(device);
}

void Renderer::Render() {
  VkSemaphore wait_semaphore = vulkan()->image_available_semaphore();
  VkSemaphore signal_semaphore = vulkan()->rendering_finished_semaphore();
  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  VkCommandBuffer cmd_buff =
      vulkan()->graphics_queue_cmd_buffers()[current_swapchain_img_];
  VkSubmitInfo submit_info = tools::inits::SubmitInfo();
  submit_info.waitSemaphoreCount = 1U;
  submit_info.pWaitSemaphores = &wait_semaphore;
  submit_info.pWaitDstStageMask = &wait_stage;
  submit_info.commandBufferCount = 1U;
  submit_info.pCommandBuffers = &cmd_buff;
  submit_info.signalSemaphoreCount = 1U;
  submit_info.pSignalSemaphores = &signal_semaphore;

  VK_CHECK_RESULT(vkQueueSubmit(vulkan()->device().graphics_queue().queue, 1U,
                                &submit_info, renderpasses_fence_));
}

void Renderer::PostRender() {
  vulkan()->swapchain().Present(vulkan()->device().present_queue(),
                                vulkan()->rendering_finished_semaphore());

  // Wait for fence here; it also ensures that the operations coming after
  // this part of flow will execute after the fence, i.e. the
  // buffers update in the next frame
  vkWaitForFences(vulkan()->device().device(), 1U, &renderpasses_fence_, true,
                  ~0ULL);
  vkResetFences(vulkan()->device().device(), 1U, &renderpasses_fence_);

  CaptureData();
}

void Renderer::SetupRenderPass(const VulkanDevice &device) {
  renderpass_ = eastl::make_unique<Renderpass>("visbuffer_full_pass");

  // Vis and derivatives buffer target
  uint32_t vis_buf_id = renderpass_->AddAttachment(
      0U, kVisBarysBufferFormat, VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // Colour buffer target
  uint32_t col_buf_id = renderpass_->AddAttachment(
      0U, vulkan()->swapchain().GetSurfaceFormat(), VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // Depth buffer target
  uint32_t depth_buf_id = renderpass_->AddAttachment(
      0U, device.depth_format(), VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  // Derivatives and barycentric coordinates
  uint32_t derivs_buf_id = renderpass_->AddAttachment(
      0U, kDerivsBufferFormat, VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Accumulation buffer
  uint32_t accum_id = renderpass_->AddAttachment(
      0U, kAccumulationFormat, VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Debug buffer
  uint32_t debug_buf_id = renderpass_->AddAttachment(
      0U, kAccumulationFormat, VK_SAMPLE_COUNT_1_BIT,
      VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
      VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // Setup first subpass
  uint32_t vis_store_sub_id =
      renderpass_->AddSubpass("vis_store", VK_PIPELINE_BIND_POINT_GRAPHICS);
  renderpass_->AddSubpassColourAttachmentRef(
      vis_store_sub_id, vis_buf_id, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  // Derivatives and barycentric coordinates
  renderpass_->AddSubpassColourAttachmentRef(
      vis_store_sub_id, derivs_buf_id,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  // Depth
  renderpass_->AddSubpassDepthAttachmentRef(
      vis_store_sub_id, depth_buf_id,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
  // Debug
  renderpass_->AddSubpassColourAttachmentRef(
      vis_store_sub_id, debug_buf_id, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  // Setup second subpass
  uint32_t vis_shade_sub_id =
      renderpass_->AddSubpass("vis_shade", VK_PIPELINE_BIND_POINT_GRAPHICS);
  renderpass_->AddSubpassColourAttachmentRef(
      vis_shade_sub_id, accum_id, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  renderpass_->AddSubpassInputAttachmentRef(
      vis_shade_sub_id, vis_buf_id, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  renderpass_->AddSubpassInputAttachmentRef(
      vis_shade_sub_id, derivs_buf_id,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  renderpass_->AddSubpassInputAttachmentRef(
      vis_shade_sub_id, depth_buf_id, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  renderpass_->AddSubpassDepthAttachmentRef(
      vis_shade_sub_id, depth_buf_id,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  uint32_t tone_sub_id =
      renderpass_->AddSubpass("tonemap", VK_PIPELINE_BIND_POINT_GRAPHICS);
  renderpass_->AddSubpassColourAttachmentRef(
      tone_sub_id, col_buf_id, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  renderpass_->AddSubpassInputAttachmentRef(
      tone_sub_id, accum_id, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  renderpass_->AddSubpassPreserveAttachmentRef(tone_sub_id, depth_buf_id);

  uint32_t skymap_sub_id =
      renderpass_->AddSubpass("skymap", VK_PIPELINE_BIND_POINT_GRAPHICS);
  renderpass_->AddSubpassColourAttachmentRef(
      skymap_sub_id, col_buf_id, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  // Depth
  renderpass_->AddSubpassDepthAttachmentRef(
      skymap_sub_id, depth_buf_id,
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  // Dependencies
  // Present to colour buffer, which is the last subpass
  renderpass_->AddSubpassDependency(
      VK_SUBPASS_EXTERNAL, skymap_sub_id, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);

  renderpass_->AddSubpassDependency(
      vis_store_sub_id, vis_shade_sub_id,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
          VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      VK_DEPENDENCY_BY_REGION_BIT);

  renderpass_->AddSubpassDependency(
      vis_shade_sub_id, tone_sub_id,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      VK_DEPENDENCY_BY_REGION_BIT);

  renderpass_->AddSubpassDependency(
      tone_sub_id, skymap_sub_id, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);

  // Final subpass to present
  renderpass_->AddSubpassDependency(
      skymap_sub_id, VK_SUBPASS_EXTERNAL,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
      VK_DEPENDENCY_BY_REGION_BIT);

  renderpass_->CreateVulkanRenderpass(device);
}

void Renderer::SetupFrameBuffers(const VulkanDevice &device) {
  // Vis buffer
  CreateFramebufferAttachment(device, kVisBarysBufferFormat,
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                              "vis_buff_and_barys", &vis_buffer_);

  // Depth buffer
  CreateFramebufferAttachment(device, device.depth_format(),
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                              "depth", &depth_buffer_);
  VkImageViewCreateInfo depth_view_create_info =
      tools::inits::ImageViewCreateInfo(
          depth_buffer_->image()->image(), VK_IMAGE_VIEW_TYPE_2D,
          depth_buffer_->image()->format(),
          {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
           VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
          {VK_IMAGE_ASPECT_DEPTH_BIT, 0U, depth_buffer_->image()->mip_levels(),
           0U, 1U});
  depth_buffer_depth_view_ = depth_buffer_->image()->CreateAdditionalImageView(
      device, depth_view_create_info);

  // Derivatives debug buffer
  CreateFramebufferAttachment(device, kDerivsBufferFormat,
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                              "derivatives", &derivs_and_barys_buffer_);

  // Accumulation buffer
  CreateFramebufferAttachment(device, kAccumulationFormat,
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                              "accumulation", &accum_buffer_);

  // Debug buffer
  VulkanTexture *debug_buffer = nullptr;
  CreateFramebufferAttachment(device, kAccumulationFormat,
                              VK_IMAGE_USAGE_SAMPLED_BIT |
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                              "debug", &debug_buffer);

  const uint32_t num_swapchain_images = vulkan()->swapchain().GetNumImages();

  for (uint32_t i = 0U; i < num_swapchain_images; i++) {
    eastl::string name;
    name.sprintf("from_swapchain_%d", i);
    eastl::unique_ptr<Framebuffer> frmbuff = eastl::make_unique<Framebuffer>(
        name, cam_->viewport().width, cam_->viewport().height, 1U,
        renderpass_.get());

    frmbuff->AddAttachment(vis_buffer_);
    frmbuff->AddAttachment(vulkan()->swapchain().images()[i]);
    frmbuff->AddAttachment(depth_buffer_);
    frmbuff->AddAttachment(derivs_and_barys_buffer_);
    frmbuff->AddAttachment(accum_buffer_);
    frmbuff->AddAttachment(debug_buffer);

    frmbuff->CreateVulkanFramebuffer(device);

    framebuffers_.push_back(eastl::move(frmbuff));
  }
}

void Renderer::CreateFramebufferAttachment(const VulkanDevice &device,
                                           VkFormat format,
                                           VkImageUsageFlags img_usage_flags,
                                           const eastl::string &name,
                                           VulkanTexture **attachment) const {
  texture_manager()->Create2DTextureFromData(
      device, name, nullptr, 0U, cam_->viewport().width,
      cam_->viewport().height, format, attachment, VK_NULL_HANDLE,
      img_usage_flags);
}

void Renderer::RegisterModel(Model &model) {
  registered_models_.push_back(&model);

  LOG("Registered model " << &model << "in VisbuffRenderer.");
}

void Renderer::SetupMaterials() {
  material_manager()->RegisterMaterialName("vis_store");
  material_manager()->RegisterMaterialName("vis_shade");
  material_manager()->RegisterMaterialName("tone");
}

void Renderer::SetupUniformBuffers(const VulkanDevice &device) {
  // Materials
  mat_consts_ = material_manager()->GetMaterialConstants();
  uint32_t num_mat_instances = material_manager()->GetMaterialInstancesCount();

  // Lights array
  eastl::vector<Light> transformed_lights;
  UpdateLights(transformed_lights);
  uint32_t num_lights = SCAST_U32(transformed_lights.size());

  // Cache some sizes
  uint32_t mat4_size = SCAST_U32(sizeof(glm::mat4));
  uint32_t mat4_group_size = mat4_size * 4U;
  uint32_t lights_array_size = (SCAST_U32(sizeof(Light)) * num_lights);
  uint32_t mat_consts_array_size =
      (SCAST_U32(sizeof(MaterialConstants)) * num_mat_instances);
  uint32_t perf_counters = SCAST_U32(sizeof(uint32_t)) * 4;

  // Main static buffer
  VulkanBufferInitInfo buff_init_info;
  buff_init_info.size = mat4_group_size + lights_array_size +
                        mat_consts_array_size + perf_counters;
  buff_init_info
      .memory_property_flags = /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  buff_init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  main_static_buff_.Init(device, buff_init_info);
}

void Renderer::SetupDescriptorPool(const VulkanDevice &device) {
  eastl::vector<VkDescriptorPoolSize> pool_sizes;

  // Uniforms
  pool_sizes.push_back(tools::inits::DescriptorPoolSize(
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, kMaxNumUniformBuffers));

  // Framebuffers
  pool_sizes.push_back(tools::inits::DescriptorPoolSize(
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      kMaxNumMatInstances * SCAST_U32(MatTextureType::size) + 10U));

  // Storage buffers
  pool_sizes.push_back(tools::inits::DescriptorPoolSize(
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, kMaxNumSSBOs));

  // Input attachments
  pool_sizes.push_back(tools::inits::DescriptorPoolSize(
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, kMaxNumInputAttachments));

  VkDescriptorPoolCreateInfo pool_create_info =
      tools::inits::DescriptrorPoolCreateInfo(
          DescSetLayoutTypes::num_items * 50U, SCAST_U32(pool_sizes.size()),
          pool_sizes.data());

  VK_CHECK_RESULT(vkCreateDescriptorPool(device.device(), &pool_create_info,
                                         nullptr, &desc_pool_));
}

void Renderer::SetupDescriptorSetAndPipeLayout(const VulkanDevice &device) {
  // Vis store descriptor set layout
  eastl::vector<std::vector<VkDescriptorSetLayoutBinding>> bindings(
      DescSetLayoutTypes::num_items);

  // Main static buffer
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kMainStaticBuffBindingPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Performance counters buffer
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kPerfCounterBufferBindingPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Lights array
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kLightsArrayBindingPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Material constants array
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kMatConstsArrayBindingPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Model matrices for all meshes
  bindings[DescSetLayoutTypes::HEAP].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kModelMatxsBufferBindPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Indirect draw buffers
  bindings[DescSetLayoutTypes::HEAP].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kIndirectDrawCmdsBindingPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Vertex buffer
  for (uint32_t i = 0U; i < SCAST_U32(VertexElementType::num_items); ++i) {
    bindings[DescSetLayoutTypes::HEAP].push_back(
        tools::inits::DescriptorSetLayoutBinding(
            kVertexBuffersBaseBindPos + i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            1U, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));
  }

  // Index buffer
  bindings[DescSetLayoutTypes::HEAP].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kIdxBufferBindPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Material IDs
  bindings[DescSetLayoutTypes::HEAP].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kMaterialIDsBufferBindPos, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1U,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Depth buffer
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kDepthBuffBindingPos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1U,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  uint32_t num_mat_instances = material_manager()->GetMaterialInstancesCount();
  // Diffuse textures as combined image samplers
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kDiffuseTexturesArrayBindingPos,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_mat_instances,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));
  // Ambient textures as combined image samplers
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kAmbientTexturesArrayBindingPos,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_mat_instances,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));
  // Specular textures as combined image samplers
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kSpecularTexturesArrayBindingPos,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_mat_instances,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));
  // Normal textures as combined image samplers
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kNormalTexturesArrayBindingPos,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_mat_instances,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));
  // Roughness textures as combined image samplers
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kRoughnessTexturesArrayBindingPos,
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_mat_instances,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Vis buffer
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kVisBufferBindingPos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1U,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Accumulation buffer
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kAccumulationBufferBindingPos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          1U, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Derivatives and bary coords buffer
  bindings[DescSetLayoutTypes::VIS_GENERIC].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kDerivsBarysBufferBindingPos, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1U,
          VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  // Skybox cubemap
  bindings[DescSetLayoutTypes::SKYBOX].push_back(
      tools::inits::DescriptorSetLayoutBinding(
          kSkyboxTextureBindingPos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          1U, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr));

  desc_set_layouts_.resize(DescSetLayoutTypes::num_items);

  for (uint32_t i = 0U; i < DescSetLayoutTypes::num_items; i++) {
    VkDescriptorSetLayoutCreateInfo set_layout_create_info =
        tools::inits::DescriptrorSetLayoutCreateInfo();
    set_layout_create_info.bindingCount = SCAST_U32(bindings[i].size());
    set_layout_create_info.pBindings = bindings[i].data();

    VK_CHECK_RESULT(
        vkCreateDescriptorSetLayout(device.device(), &set_layout_create_info,
                                    nullptr, &desc_set_layouts_[i]));

    LOG("Desc set layout: " << desc_set_layouts_[i] << " b count: "
                            << set_layout_create_info.bindingCount);
  }

  eastl::array<VkDescriptorSetLayout, SetTypes::num_items> local_layouts = {
      desc_set_layouts_[DescSetLayoutTypes::VIS_GENERIC],
      desc_set_layouts_[DescSetLayoutTypes::SKYBOX]};
  VkDescriptorSetAllocateInfo set_allocate_info =
      tools::inits::DescriptorSetAllocateInfo(
          desc_pool_, SCAST_U32(local_layouts.size()), local_layouts.data());

  VK_CHECK_RESULT(vkAllocateDescriptorSets(device.device(), &set_allocate_info,
                                           desc_sets_.data()));

  // Create pipeline layouts
  pipe_layouts_.resize(PipeLayoutTypes::num_items);

  // Push constant for the meshes ID
  VkPushConstantRange push_const_range = {VK_SHADER_STAGE_VERTEX_BIT, 0U,
                                          SCAST_U32(sizeof(uint32_t))};

  VkPipelineLayoutCreateInfo pipe_layout_create_info =
      tools::inits::PipelineLayoutCreateInfo(
          DescSetLayoutTypes::num_items, // Desc set layouts up to VISBUFF
          desc_set_layouts_.data(), 1U, &push_const_range);

  VK_CHECK_RESULT(
      vkCreatePipelineLayout(device.device(), &pipe_layout_create_info, nullptr,
                             &pipe_layouts_[PipeLayoutTypes::VPASS]));
}

void Renderer::SetupDescriptorSets(const VulkanDevice &device) {
  // Update the descriptor set
  eastl::vector<VkWriteDescriptorSet> write_desc_sets;

  // Cache some sizes
  uint32_t num_mat_instances = material_manager()->GetMaterialInstancesCount();
  uint32_t num_lights = lights_manager()->GetNumLights();
  uint32_t mat4_size = SCAST_U32(sizeof(glm::mat4));
  uint32_t mat4_group_size = mat4_size * 4U;
  uint32_t lights_array_size = (SCAST_U32(sizeof(Light)) * num_lights);
  uint32_t mat_consts_array_size =
      (SCAST_U32(sizeof(MaterialConstants)) * num_mat_instances);
  uint32_t perf_counters = SCAST_U32(sizeof(uint32_t)) * 4;

  // Main static buffer
  VkDescriptorBufferInfo desc_main_static_buff_info =
      main_static_buff_.GetDescriptorBufferInfo(mat4_group_size);
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kMainStaticBuffBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &desc_main_static_buff_info,
      nullptr));

  // Lights array
  VkDescriptorBufferInfo desc_lights_array_info =
      main_static_buff_.GetDescriptorBufferInfo(lights_array_size,
                                                mat4_group_size);
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kLightsArrayBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &desc_lights_array_info,
      nullptr));

  // Material constants array
  VkDescriptorBufferInfo desc_mat_consts_info =
      main_static_buff_.GetDescriptorBufferInfo(
          mat_consts_array_size, mat4_group_size + lights_array_size);
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kMatConstsArrayBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &desc_mat_consts_info,
      nullptr));

  // Perf buffer
  VkDescriptorBufferInfo desc_perf_counters =
      main_static_buff_.GetDescriptorBufferInfo(
          perf_counters,
          mat4_group_size + lights_array_size + mat_consts_array_size);
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kPerfCounterBufferBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &desc_perf_counters,
      nullptr));

  // Depth buffer
  VkDescriptorImageInfo depth_buff_img_info =
      depth_buffer_->image()->GetDescriptorImageInfo(nearest_sampler_);
  depth_buff_img_info.imageView = *depth_buffer_depth_view_;
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kDepthBuffBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &depth_buff_img_info, nullptr,
      nullptr));

  eastl::vector<VkDescriptorImageInfo> diff_descs_image_infos;
  material_manager()->GetDescriptorImageInfosByType(MatTextureType::DIFFUSE,
                                                    diff_descs_image_infos);

  // Diffuse textures as combined image samplers
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kDiffuseTexturesArrayBindingPos, 0U,
      SCAST_U32(diff_descs_image_infos.size()),
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, diff_descs_image_infos.data(),
      nullptr, nullptr));

  eastl::vector<VkDescriptorImageInfo> amb_descs_image_infos;
  material_manager()->GetDescriptorImageInfosByType(MatTextureType::AMBIENT,
                                                    amb_descs_image_infos);

  // Ambient textures as combined image samplers
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kAmbientTexturesArrayBindingPos, 0U,
      SCAST_U32(amb_descs_image_infos.size()),
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, amb_descs_image_infos.data(),
      nullptr, nullptr));

  eastl::vector<VkDescriptorImageInfo> spec_descs_image_infos;
  material_manager()->GetDescriptorImageInfosByType(MatTextureType::SPECULAR,
                                                    spec_descs_image_infos);

  // Specular textures as combined image samplers
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kSpecularTexturesArrayBindingPos, 0U,
      SCAST_U32(spec_descs_image_infos.size()),
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, spec_descs_image_infos.data(),
      nullptr, nullptr));

  eastl::vector<VkDescriptorImageInfo> rough_descs_image_infos;
  material_manager()->GetDescriptorImageInfosByType(
      MatTextureType::SPECULAR_HIGHLIGHT, rough_descs_image_infos);

  // Roughness textures as combined image samplers
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kRoughnessTexturesArrayBindingPos, 0U,
      SCAST_U32(rough_descs_image_infos.size()),
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, rough_descs_image_infos.data(),
      nullptr, nullptr));

  eastl::vector<VkDescriptorImageInfo> norm_descs_image_infos;
  material_manager()->GetDescriptorImageInfosByType(MatTextureType::NORMAL,
                                                    norm_descs_image_infos);

  // Normal textures as combined image samplers
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kNormalTexturesArrayBindingPos, 0U,
      SCAST_U32(norm_descs_image_infos.size()),
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, norm_descs_image_infos.data(),
      nullptr, nullptr));

  // Visibility buffer
  VkDescriptorImageInfo desc_vis_buff_info =
      vis_buffer_->image()->GetDescriptorImageInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kVisBufferBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &desc_vis_buff_info, nullptr,
      nullptr));

  // Accumulation buffer
  VkDescriptorImageInfo accum_buff_img_info =
      accum_buffer_->image()->GetDescriptorImageInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kAccumulationBufferBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &accum_buff_img_info, nullptr,
      nullptr));

  // Derivatives and bary coords buffer
  VkDescriptorImageInfo derivs_barys_buff_img_info =
      derivs_and_barys_buffer_->image()->GetDescriptorImageInfo();
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::VIS_GENERIC], kDerivsBarysBufferBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &derivs_barys_buff_img_info, nullptr,
      nullptr));

  VkDescriptorImageInfo skybox_image_info =
      skybox_texture_->image()->GetDescriptorImageInfo(aniso_edge_sampler_);

  // Skybox texture
  write_desc_sets.push_back(tools::inits::WriteDescriptorSet(
      desc_sets_[SetTypes::SKYBOX], kSkyboxTextureBindingPos, 0U, 1U,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &skybox_image_info, nullptr,
      nullptr));

  vkUpdateDescriptorSets(device.device(), SCAST_U32(write_desc_sets.size()),
                         write_desc_sets.data(), 0U, nullptr);
}

void Renderer::UpdatePVMatrices() {
  proj_mat_ = cam_->projection_mat();
  view_mat_ = cam_->view_mat();
  inv_proj_mat_ = glm::inverse(proj_mat_);
  inv_view_mat_ = glm::inverse(view_mat_);
}

void Renderer::SetupCommandBuffers() {
  // Cache common settings to all command buffers
  VkCommandBufferBeginInfo cmd_buff_begin_info =
      tools::inits::CommandBufferBeginInfo(
          VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
  cmd_buff_begin_info.pInheritanceInfo = nullptr;

  std::vector<VkClearValue> clear_values;
  VkClearValue clear_value;
  clear_value.color = {{0.f, 0.f, 0.f, 0.f}};
  clear_values.push_back(clear_value);
  clear_values.push_back(clear_value);
  clear_value.depthStencil = {1.f, 0U};
  clear_values.push_back(clear_value);
  clear_value.color = {{0.f, 0.f, 0.f, 0.f}};
  clear_values.push_back(clear_value);
  clear_values.push_back(clear_value);
  clear_values.push_back(clear_value);

  // Record command buffers
  const eastl::vector<VkCommandBuffer> &graphics_buffs =
      vulkan()->graphics_queue_cmd_buffers();
  uint32_t num_swapchain_images = vulkan()->swapchain().GetNumImages();
  for (uint32_t i = 0U; i < num_swapchain_images; i++) {
    VK_CHECK_RESULT(
        vkBeginCommandBuffer(graphics_buffs[i], &cmd_buff_begin_info));

    renderpass_->BeginRenderpass(
        graphics_buffs[i], VK_SUBPASS_CONTENTS_INLINE, framebuffers_[i].get(),
        {0U, 0U, cam_->viewport().width, cam_->viewport().height},
        SCAST_U32(clear_values.size()), clear_values.data());

    vis_store_material_->BindPipeline(graphics_buffs[i],
                                      VK_PIPELINE_BIND_POINT_GRAPHICS);

    vkCmdBindDescriptorSets(graphics_buffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipe_layouts_[PipeLayoutTypes::VPASS], 0U,
                            DescSetLayoutTypes::HEAP, desc_sets_.data(), 0U,
                            nullptr);

    for (eastl::vector<Model *>::iterator itor = registered_models_.begin();
         itor != registered_models_.end(); ++itor) {
      (*itor)->BindVertexBuffer(graphics_buffs[i]);
      (*itor)->BindIndexBuffer(graphics_buffs[i]);
      (*itor)->RenderMeshesByMaterial(graphics_buffs[i],
                                      pipe_layouts_[PipeLayoutTypes::VPASS],
                                      DescSetLayoutTypes::HEAP);
    }

    // Fullscreen pass
    renderpass_->NextSubpass(graphics_buffs[i], VK_SUBPASS_CONTENTS_INLINE);

    vis_shade_material_->BindPipeline(graphics_buffs[i],
                                      VK_PIPELINE_BIND_POINT_GRAPHICS);

    fullscreenquad_->BindVertexBuffer(graphics_buffs[i]);
    fullscreenquad_->BindIndexBuffer(graphics_buffs[i]);

    vkCmdDrawIndexed(graphics_buffs[i], 6U, 1U, 0U, 0U, 0U);

    // Tonemapping pass
    renderpass_->NextSubpass(graphics_buffs[i], VK_SUBPASS_CONTENTS_INLINE);

    tonemap_material_->BindPipeline(graphics_buffs[i],
                                    VK_PIPELINE_BIND_POINT_GRAPHICS);

    vkCmdDrawIndexed(graphics_buffs[i], 6U, 1U, 0U, 0U, 0U);

    // Skybox pass
    renderpass_->NextSubpass(graphics_buffs[i], VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindDescriptorSets(graphics_buffs[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipe_layouts_[PipeLayoutTypes::VPASS], 2U, 1U,
                            &desc_sets_[SetTypes::SKYBOX], 0U, nullptr);

    skybox_material_->BindPipeline(graphics_buffs[i],
                                   VK_PIPELINE_BIND_POINT_GRAPHICS);

    cube_->BindVertexBuffer(graphics_buffs[i]);
    cube_->BindIndexBuffer(graphics_buffs[i]);

    vkCmdDrawIndexed(graphics_buffs[i], 36U, 1U, 0U, 0U, 0U);

    renderpass_->EndRenderpass(graphics_buffs[i]);

    VK_CHECK_RESULT(vkEndCommandBuffer(graphics_buffs[i]));
  }
}

void Renderer::SetupMaterialPipelines(const VulkanDevice &device,
                                      const VertexSetup &g_store_vertex_setup) {
  eastl::vector<VertexElement> vtx_layout;
  vtx_layout.push_back(VertexElement(VertexElementType::POSITION,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));

  VertexSetup vertex_setup_quads(vtx_layout);

  // Setup visibility shade material
  eastl::unique_ptr<MaterialShader> vis_shade_frag =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath +
                                             "vis_shade_amd.frag",
                                         "main", ShaderTypes::FRAGMENT);
  // vis_shade_frag->ProfileBandwidth(ProfileStage::SECOND);

  eastl::unique_ptr<MaterialShader> vis_shade_vert =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath +
                                             "vis_shade.vert",
                                         "main", ShaderTypes::VERTEX);

  uint32_t num_materials = material_manager()->GetMaterialInstancesCount();
  vis_shade_frag->AddSpecialisationEntry(
      kNumMaterialsSpecConstPos, SCAST_U32(sizeof(uint32_t)), &num_materials);
  uint32_t num_lights = lights_manager()->GetNumLights();
  vis_shade_frag->AddSpecialisationEntry(
      kNumLightsSpecConstPos, SCAST_U32(sizeof(uint32_t)), &num_lights);
  vis_shade_vert->AddSpecialisationEntry(
      kNumMaterialsSpecConstPos, SCAST_U32(sizeof(uint32_t)), &num_materials);
  vis_shade_vert->AddSpecialisationEntry(
      kNumLightsSpecConstPos, SCAST_U32(sizeof(uint32_t)), &num_lights);

  eastl::unique_ptr<MaterialBuilder> builder_shade =
      eastl::make_unique<MaterialBuilder>(
          vertex_setup_quads, "vis_shade",
          pipe_layouts_[PipeLayoutTypes::VPASS], renderpass_->GetVkRenderpass(),
          VK_FRONT_FACE_COUNTER_CLOCKWISE, 1U, cam_->viewport());

  builder_shade->AddColorBlendAttachment(
      VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, 0xf);
  // builder_shade->AddColorBlendAttachment(
  // VK_FALSE,
  // VK_BLEND_FACTOR_ONE,
  // VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
  // VK_BLEND_OP_ADD,
  // VK_BLEND_FACTOR_ONE,
  // VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
  // VK_BLEND_OP_ADD,
  // 0xf);

  float blend_constants[4U] = {1.f, 1.f, 1.f, 1.f};
  builder_shade->AddColorBlendStateCreateInfo(VK_FALSE, VK_LOGIC_OP_SET,
                                              blend_constants);

  builder_shade->AddShader(eastl::move(vis_shade_vert));
  builder_shade->AddShader(eastl::move(vis_shade_frag));
  builder_shade->SetDepthTestEnable(VK_TRUE);
  builder_shade->SetDepthCompareOp(VK_COMPARE_OP_ALWAYS);
  builder_shade->SetStencilTestEnable(VK_TRUE);
  builder_shade->SetStencilStateFront(tools::inits::StencilOpState(
      VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_KEEP,
      VK_COMPARE_OP_EQUAL, ~0U, 0U, 1U));

  vis_shade_material_ =
      material_manager()->CreateMaterial(device, eastl::move(builder_shade));

  // Setup visibility storage material
  eastl::unique_ptr<MaterialShader> vis_store_frag =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath +
                                             "vis_store_amd.frag",
                                         "main", ShaderTypes::FRAGMENT);
  // vis_store_frag->ProfileBandwidth(ProfileStage::FIRST);

  eastl::unique_ptr<MaterialShader> vis_store_vert =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath +
                                             "vis_store_amd.vert",
                                         "main", ShaderTypes::VERTEX);

  vis_store_vert->AddSpecialisationEntry(
      kNumMaterialsSpecConstPos, SCAST_U32(sizeof(uint32_t)), &num_materials);
  vis_store_vert->AddSpecialisationEntry(
      kNumLightsSpecConstPos, SCAST_U32(sizeof(uint32_t)), &num_lights);

  eastl::unique_ptr<MaterialBuilder> builder_store =
      eastl::make_unique<MaterialBuilder>(
          g_store_vertex_setup, "vis_store",
          pipe_layouts_[PipeLayoutTypes::VPASS], renderpass_->GetVkRenderpass(),
          VK_FRONT_FACE_COUNTER_CLOCKWISE, 0U, cam_->viewport());

  builder_store->AddColorBlendAttachment(
      VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, 0xf);
  builder_store->AddColorBlendAttachment(
      VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, 0xf);
  builder_store->AddColorBlendAttachment(
      VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, 0xf);
  builder_store->AddColorBlendStateCreateInfo(VK_FALSE, VK_LOGIC_OP_SET,
                                              blend_constants);
  builder_store->AddShader(eastl::move(vis_store_vert));
  builder_store->AddShader(eastl::move(vis_store_frag));
  builder_store->SetDepthTestEnable(VK_TRUE);
  builder_store->SetDepthWriteEnable(VK_TRUE);
  builder_store->SetDepthCompareOp(VK_COMPARE_OP_LESS);
  builder_store->SetStencilTestEnable(VK_TRUE);
  builder_store->SetStencilStateFront(tools::inits::StencilOpState(
      VK_STENCIL_OP_KEEP, VK_STENCIL_OP_REPLACE, VK_STENCIL_OP_KEEP,
      VK_COMPARE_OP_ALWAYS, ~0U, ~0U, 1U));

  vis_store_material_ =
      material_manager()->CreateMaterial(device, eastl::move(builder_store));

  // Setup tonemap material
  eastl::unique_ptr<MaterialShader> tone_frag =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath +
                                             "tonemapping_visbuff.frag",
                                         "main", ShaderTypes::FRAGMENT);

  tone_frag->AddSpecialisationEntry(kTonemapExposureSpecConstPos,
                                    SCAST_U32(sizeof(float)),
                                    SCAST_CVOIDPTR(&kTonemapExposure));

  eastl::unique_ptr<MaterialShader> tone_vert =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath +
                                             "tonemapping.vert",
                                         "main", ShaderTypes::VERTEX);

  eastl::unique_ptr<MaterialBuilder> builder_tone =
      eastl::make_unique<MaterialBuilder>(
          vertex_setup_quads, "tone", pipe_layouts_[PipeLayoutTypes::VPASS],
          renderpass_->GetVkRenderpass(), VK_FRONT_FACE_COUNTER_CLOCKWISE, 2U,
          cam_->viewport());

  builder_tone->AddColorBlendAttachment(
      VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, 0xf);
  builder_tone->AddColorBlendStateCreateInfo(VK_FALSE, VK_LOGIC_OP_SET,
                                             blend_constants);
  builder_tone->AddShader(eastl::move(tone_vert));
  builder_tone->AddShader(eastl::move(tone_frag));

  tonemap_material_ =
      material_manager()->CreateMaterial(device, eastl::move(builder_tone));

  // Setup skybox material
  eastl::unique_ptr<MaterialShader> skybox_frag =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath + "skybox.frag",
                                         "main", ShaderTypes::FRAGMENT);

  eastl::unique_ptr<MaterialShader> skybox_vert =
      eastl::make_unique<MaterialShader>(kBaseShaderAssetsPath + "skybox.vert",
                                         "main", ShaderTypes::VERTEX);

  eastl::unique_ptr<MaterialBuilder> builder_skybox =
      eastl::make_unique<MaterialBuilder>(
          vertex_setup_quads, "skybox", pipe_layouts_[PipeLayoutTypes::VPASS],
          renderpass_->GetVkRenderpass(), VK_FRONT_FACE_COUNTER_CLOCKWISE, 3U,
          cam_->viewport());

  builder_skybox->AddColorBlendAttachment(
      VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      VK_BLEND_OP_ADD, 0xf);
  builder_skybox->AddColorBlendStateCreateInfo(VK_FALSE, VK_LOGIC_OP_SET,
                                               blend_constants);
  builder_skybox->AddShader(eastl::move(skybox_vert));
  builder_skybox->AddShader(eastl::move(skybox_frag));
  builder_skybox->SetDepthWriteEnable(VK_FALSE);
  builder_skybox->SetDepthTestEnable(VK_TRUE);

  skybox_material_ =
      material_manager()->CreateMaterial(device, eastl::move(builder_skybox));
}

void Renderer::SetupFullscreenQuad(const VulkanDevice &device) {
  eastl::vector<VertexElement> vtx_layout;
  vtx_layout.push_back(VertexElement(VertexElementType::POSITION,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));

  VertexSetup vertex_setup_quads(vtx_layout);

  // Group all vertex data together
  ModelBuilder model_builder(vertex_setup_quads, desc_pool_);

  // Coordinate space is right-handed with Y pointing downwards
  // and Z away from the screen
  Vertex vtx;
  vtx.pos = {-1.f, -1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {-1.f, 1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {1.f, 1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {1.f, -1.f, 1.f};
  model_builder.AddVertex(vtx);

  model_builder.AddIndex(0U);
  model_builder.AddIndex(1U);
  model_builder.AddIndex(2U);
  model_builder.AddIndex(0U);
  model_builder.AddIndex(2U);
  model_builder.AddIndex(3U);

  Mesh quad_mesh(0U, 6U, 0U, 0U);

  model_builder.AddMesh(&quad_mesh);

  model_manager()->CreateModel(device, "fullscreenquad", model_builder,
                               &fullscreenquad_);
}

void Renderer::UpdateLights(eastl::vector<Light> &transformed_lights) {
  transformed_lights = lights_manager()->TransformLights(view_mat_);
}

void Renderer::ReloadAllShaders() {
  material_manager()->ReloadAllShaders(vulkan()->device());

  SetupCommandBuffers();
}

void Renderer::OutputPerformanceDataToFile() const {
  if (mem_perf_data_reads_.size() == 0 && mem_perf_data_writes_.size() == 0) {
    LOG("Performance data absent; performance report won't be output.");
    return;
  }

  const float kMebi = 1048576.f;
  std::ofstream ofs(STR(PERF_DATA_FOLDER) "/perf_report_visbuff.txt");
  eastl::vector<FrameMemoryData> average_reads(mem_perf_data_writes_.size());
  eastl::vector<FrameMemoryData> average_writes(mem_perf_data_writes_.size());

  ofs << "R;Wvis+bary;Wderivs;Wdepth(1stPass);ReadMaps;ReadVis,ReadDerivs,"
         "ReadDepth,"
         "Writes\n";

  for (eastl_size_t i = 0; i < mem_perf_data_reads_.size(); ++i) {
    average_reads[i] = {0, 0};
    average_writes[i] = {0, 0};

    // Calculate avg
    for (eastl_size_t j = 0; j < kFramesCaptureNum; ++j) {
      average_reads[i].first_frame += mem_perf_data_reads_[i][j].first_frame;
      average_reads[i].second_frame += mem_perf_data_reads_[i][j].second_frame;
      average_writes[i].first_frame += mem_perf_data_writes_[i][j].first_frame;
      average_writes[i].second_frame +=
          mem_perf_data_writes_[i][j].second_frame;
    }
    average_reads[i].first_frame /= kFramesCaptureNum;
    average_reads[i].second_frame /= kFramesCaptureNum;
    average_writes[i].first_frame /= kFramesCaptureNum;
    average_writes[i].second_frame /= kFramesCaptureNum;

    // Output 1st read; will be zero
    ofs << average_reads[i].first_frame << ";";
    // Output 1st write
    ofs << ((SCAST_FLOAT(average_writes[i].first_frame) * 8.f) / kMebi) << ";";
    ofs << ((SCAST_FLOAT(average_writes[i].first_frame) * 8.f) / kMebi) << ";";
    ofs << ((SCAST_FLOAT(average_writes[i].first_frame) * 5.f) / kMebi) << ";";

    float reads_single_map = SCAST_FLOAT(average_reads[i].second_frame / 8U);
    // Output 2nd read maps
    ofs << ((reads_single_map * 20.f) / kMebi) << ";";
    // Output 2nd read visibility and bary coords buffer
    ofs << ((reads_single_map * 8.f) / kMebi) << ";";
    // Output 2nd read derivatives buffer
    ofs << ((reads_single_map * 8.f) / kMebi) << ";";
    // Output 2nd read depth buffer
    ofs << ((reads_single_map * 5.f) / kMebi) << ";";
    // Output 2nd writes
    ofs << ((SCAST_FLOAT(average_writes[i].second_frame) * 8.f) / kMebi)
        << "\n";
  }
}

void Renderer::CaptureData() {
  // Read back performance data
  if (capturing_enabled_) {
    if (num_captures_ < num_captures_to_collect_) {
      // When entering this branch, assume that the code which enables capturing
      // of data for 20 frames also pushes a new array in the vector of arrays
      if (frames_captured_ < kFramesCaptureNum) {

        void *mapped = nullptr;
        union BufferData {
          uint32_t data_uint[4U];
          FrameMemoryData data_struct[2U];
        } mapped_data;

        // Position the camera at the position for capturing
        main_static_buff_.Map(vulkan()->device(), &mapped, sizeof(uint32_t) * 4,
                              main_static_buff_.size() -
                                  (sizeof(uint32_t) * 4));
        uint32_t *mapped_u32 = static_cast<uint32_t *>(mapped);

        mapped_data.data_uint[0] = *mapped_u32;
        mapped_data.data_uint[1] = *(mapped_u32 + 1U);
        mapped_data.data_uint[2] = *(mapped_u32 + 2U);
        mapped_data.data_uint[3] = *(mapped_u32 + 3U);

        main_static_buff_.Unmap(vulkan()->device());

        mem_perf_data_reads_.back()[frames_captured_] =
            mapped_data.data_struct[0U];
        mem_perf_data_writes_.back()[frames_captured_] =
            mapped_data.data_struct[1U];

        frames_captured_++;
      } else {
        frames_captured_ = 0U;
        num_captures_++;
        if (capture_screenshot_ || capturing_from_positions_enabled_) {
          std::stringstream filename;
          filename << STR(SCREENS_FOLDER) "screen_capture"
                   << mem_perf_data_reads_.size() << ".ppm";
          CaptureScreenshot(filename.str().c_str());
          capture_screenshot_ = false;
        }
        if (num_captures_ < num_captures_to_collect_) {
          mem_perf_data_writes_.push_back();
          mem_perf_data_reads_.push_back();
        }
      }
    }
    if (num_captures_ >= num_captures_to_collect_) {
      capturing_enabled_ = false;
      capturing_from_positions_enabled_ = false;
      frames_captured_ = 0U;
      num_captures_ = 0U;
      num_captures_to_collect_ = 0U;
    }
  }
}

void Renderer::CaptureScreenshot(const eastl::string &filename) const {
  // Check if blitting is supported
  bool supports_blit = true;

  // Get format properties for the swapchain color format
  VkFormatProperties format_props;

  // Check if the device supports blitting from optimal layout images
  // (the swapchain images are in optimal layout)
  vkGetPhysicalDeviceFormatProperties(vulkan()->device().physical_device(),
                                      vulkan()->swapchain().GetSurfaceFormat(),
                                      &format_props);
  if (!(format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
    ELOG_WARN("Device does not support blitting from optimal tiled images, "
              "using copy instead of blit!");
    supports_blit = false;
  }

  // Check if the device supports blitting to linear images

  vkGetPhysicalDeviceFormatProperties(vulkan()->device().physical_device(),
                                      VK_FORMAT_R8G8B8A8_UNORM, &format_props);
  if (!(format_props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
    ELOG_WARN("Device does not support blitting to linear tiled images, using "
              "copy instead of blit!");
    supports_blit = false;
  }

  // Create the image
  VkImageCreateInfo image_create_info = tools::inits::ImageCreateInfo(
      0U, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_SRGB,
      {vulkan()->swapchain().width(), vulkan()->swapchain().height(), 1U}, 1U,
      1U, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_LINEAR,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, 0U, nullptr,
      VK_IMAGE_LAYOUT_UNDEFINED);

  VulkanImageInitInfo image_init_info;
  image_init_info.create_info = image_create_info;
  image_init_info.create_view = CreateView::NO;
  image_init_info.memory_properties_flags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  eastl::unique_ptr<VulkanImage> dst_image = eastl::make_unique<VulkanImage>();
  dst_image->Init(vulkan()->device(), image_init_info);

  // Create a separate command buffer for submitting
  // image barriers
  VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
      vulkan()->device().graphics_queue().cmd_pool,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1U};
  VkCommandBuffer copy_cmd_buff_ = VK_NULL_HANDLE;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(
      vulkan()->device().device(), &cmd_buffer_allocate_info, &copy_cmd_buff_));

  // Use the separate command buffer for texture loading
  VkCommandBufferBeginInfo cmd_buff_begin_info =
      tools::inits::CommandBufferBeginInfo();
  VK_CHECK_RESULT(vkBeginCommandBuffer(copy_cmd_buff_, &cmd_buff_begin_info));

  // Use an image barrier to setup an optimal image layout for the copy
  // for both the swapchain image and the
  VkImageSubresourceRange subresource_range;
  subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresource_range.baseMipLevel = 0U;
  subresource_range.levelCount = 1U;
  subresource_range.baseArrayLayer = 0U;
  subresource_range.layerCount = 1U;

  tools::SetImageLayout(
      copy_cmd_buff_, *dst_image.get(), VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

  VulkanImage *src_image =
      vulkan()->swapchain().images()[current_swapchain_img_]->image();
  VkImageLayout prev_src_img_layout = src_image->layout();

  tools::SetImageLayout(
      copy_cmd_buff_, *src_image, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_MEMORY_READ_BIT,
      VK_ACCESS_TRANSFER_READ_BIT, subresource_range);

  // If source and destination support blit, use blit as this also does
  // automatic format conversion (e.g. from BGR to RGB)
  if (supports_blit) {
    // Define the region to blit (we will blit the whole swapchain image)
    VkOffset3D blitSize;
    blitSize.x = static_cast<int32_t>(vulkan()->swapchain().width());
    blitSize.y = static_cast<int32_t>(vulkan()->swapchain().height());
    blitSize.z = 1;
    VkImageBlit blit_region = {};
    blit_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.srcSubresource.layerCount = 1;
    blit_region.srcOffsets[1] = blitSize;
    blit_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit_region.dstSubresource.layerCount = 1;
    blit_region.dstOffsets[1] = blitSize;

    // Issue the blit command
    vkCmdBlitImage(copy_cmd_buff_, src_image->image(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image->image(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region,
                   VK_FILTER_NEAREST);
  } else {
    // Otherwise use image copy (requires to manually flip components)
    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width =
        static_cast<int32_t>(vulkan()->swapchain().width());
    imageCopyRegion.extent.height =
        static_cast<int32_t>(vulkan()->swapchain().height());
    imageCopyRegion.extent.depth = 1;

    // Issue the copy command
    vkCmdCopyImage(copy_cmd_buff_, src_image->image(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image->image(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
  }

  tools::SetImageLayout(copy_cmd_buff_, *dst_image.get(),
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_GENERAL, subresource_range);
  tools::SetImageLayout(
      copy_cmd_buff_, *src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_READ_BIT,
      VK_ACCESS_MEMORY_READ_BIT, subresource_range);

  VK_CHECK_RESULT(vkEndCommandBuffer(copy_cmd_buff_));

  // Use a fence to make sure that the copying operations have completed
  // and then submit the command buffer
  VkFenceCreateInfo fence_create_info = tools::inits::FenceCreateInfo();
  VkFence copy_fence = VK_NULL_HANDLE;
  VK_CHECK_RESULT(vkCreateFence(vulkan()->device().device(), &fence_create_info,
                                nullptr, &copy_fence));

  VkSubmitInfo submit_info = tools::inits::SubmitInfo();
  submit_info.waitSemaphoreCount = 0U;
  submit_info.pWaitSemaphores = nullptr;
  submit_info.pWaitDstStageMask = nullptr;
  submit_info.commandBufferCount = 1U;
  submit_info.pCommandBuffers = &copy_cmd_buff_;
  submit_info.signalSemaphoreCount = 0U;
  submit_info.pSignalSemaphores = nullptr;

  vkQueueSubmit(vulkan()->device().graphics_queue().queue, 1U, &submit_info,
                copy_fence);

  VK_CHECK_RESULT(vkWaitForFences(vulkan()->device().device(), 1U, &copy_fence,
                                  VK_TRUE, UINT64_MAX));

  // Cleanup the staging resources
  vkDestroyFence(vulkan()->device().device(), copy_fence, nullptr);
  copy_fence = VK_NULL_HANDLE;

  // Get layout of the image (including row pitch)
  VkImageSubresource subresource{};
  subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subresource_layout;

  vkGetImageSubresourceLayout(vulkan()->device().device(), dst_image->image(),
                              &subresource, &subresource_layout);

  // Map image memory so we can start copying from it
  const char *data = nullptr;
  vkMapMemory(vulkan()->device().device(), dst_image->memory(), 0,
              VK_WHOLE_SIZE, 0, (void **)&data);

  data += subresource_layout.offset;

  {
    std::ofstream file(filename.c_str(), std::ofstream::binary);

    // ppm header
    file << "P6\n"
         << vulkan()->swapchain().width() << "\n"
         << vulkan()->swapchain().height() << "\n"
         << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which
    // does automatic conversion), we'll have to manually swizzle color
    // components
    bool colorSwizzle = false;
    // Check if source is BGR
    // Note: Not complete, only contains most common and basic BGR surface
    // formats for demonstation purposes
    if (!supports_blit) {
      std::vector<VkFormat> formatsBGR = {VK_FORMAT_B8G8R8A8_SRGB,
                                          VK_FORMAT_B8G8R8A8_UNORM,
                                          VK_FORMAT_B8G8R8A8_SNORM};
      colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(),
                                vulkan()->swapchain().GetSurfaceFormat()) !=
                      formatsBGR.end());
    }

    // ppm binary pixel data
    for (uint32_t y = 0; y < vulkan()->swapchain().height(); y++) {
      const uint32_t *row = reinterpret_cast<const uint32_t *>(data);
      for (uint32_t x = 0; x < vulkan()->swapchain().width(); x++) {
        if (colorSwizzle) {
          file.write(reinterpret_cast<const char *>(row) + 2, 1);
          file.write(reinterpret_cast<const char *>(row) + 1, 1);
          file.write(reinterpret_cast<const char *>(row), 1);
        } else {
          file.write(reinterpret_cast<const char *>(row), 3);
        }
        row++;
      }
      data += subresource_layout.rowPitch;
    }
  }

  vkUnmapMemory(vulkan()->device().device(), dst_image->memory());

  LOG("Screenshot " << filename.c_str() << " saved to disk.");
}

void Renderer::CaptureBandwidthDataFromPositions(
    const eastl::array<glm::vec3, kCapturesNum> &sample_positions,
    const eastl::array<glm::vec3, kCapturesNum> &sample_directions) const {
  capturing_from_positions_enabled_ = true;
  capturing_enabled_ = true;
  capture_screenshot_ = true;
  num_captures_ = 0U;
  frames_captured_ = 0U;
  num_captures_to_collect_ = kCapturesNum;
  mem_perf_data_writes_.push_back();
  mem_perf_data_reads_.push_back();

  camera_sample_positions_ = sample_positions;
  camera_sample_directions_ = sample_directions;
}

void Renderer::CaptureBandwidthDataAtPosition() const {
  capturing_enabled_ = true;
  capture_screenshot_ = true;
  num_captures_ = 0U;
  frames_captured_ = 0U;
  num_captures_to_collect_ = 1U;
  mem_perf_data_writes_.push_back();
  mem_perf_data_reads_.push_back();
}

void Renderer::CreateCubeMesh(const VulkanDevice &device) {
  eastl::vector<VertexElement> vtx_layout;
  vtx_layout.push_back(VertexElement(VertexElementType::POSITION,
                                     SCAST_U32(sizeof(glm::vec3)),
                                     VK_FORMAT_R32G32B32_SFLOAT));

  VertexSetup vertex_setup_quads(vtx_layout);

  // Group all vertex data together
  ModelBuilder model_builder(vertex_setup_quads, desc_pool_);

  // Coordinate space is right-handed with Y pointing downwards
  // and Z away from the screen
  Vertex vtx;
  vtx.pos = {-1.f, -1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {-1.f, 1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {1.f, 1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {1.f, -1.f, 1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {-1.f, -1.f, -1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {-1.f, 1.f, -1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {1.f, 1.f, -1.f};
  model_builder.AddVertex(vtx);
  vtx.pos = {1.f, -1.f, -1.f};
  model_builder.AddVertex(vtx);

  // Front face
  model_builder.AddIndex(0U);
  model_builder.AddIndex(1U);
  model_builder.AddIndex(2U);
  model_builder.AddIndex(0U);
  model_builder.AddIndex(2U);
  model_builder.AddIndex(3U);

  // Back face
  model_builder.AddIndex(7U);
  model_builder.AddIndex(6U);
  model_builder.AddIndex(5U);
  model_builder.AddIndex(7U);
  model_builder.AddIndex(5U);
  model_builder.AddIndex(4U);

  // Left face
  model_builder.AddIndex(3U);
  model_builder.AddIndex(2U);
  model_builder.AddIndex(6U);
  model_builder.AddIndex(3U);
  model_builder.AddIndex(6U);
  model_builder.AddIndex(7U);

  // Right face
  model_builder.AddIndex(4U);
  model_builder.AddIndex(5U);
  model_builder.AddIndex(1U);
  model_builder.AddIndex(4U);
  model_builder.AddIndex(1U);
  model_builder.AddIndex(0U);

  // Bottom face
  model_builder.AddIndex(1U);
  model_builder.AddIndex(5U);
  model_builder.AddIndex(6U);
  model_builder.AddIndex(1U);
  model_builder.AddIndex(6U);
  model_builder.AddIndex(2U);

  // Top face
  model_builder.AddIndex(4U);
  model_builder.AddIndex(0U);
  model_builder.AddIndex(3U);
  model_builder.AddIndex(4U);
  model_builder.AddIndex(3U);
  model_builder.AddIndex(7U);

  Mesh quad_mesh(0U, SCAST_U32(model_builder.indices_data().size()), 0U, 0U);

  model_builder.AddMesh(&quad_mesh);

  model_manager()->CreateModel(device, "cube", model_builder, &cube_);
}

} // namespace vks
