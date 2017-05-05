#ifndef VKS_MODELMANAGER
#define VKS_MODELMANAGER

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <assimp/postprocess.h>
#include <renderer_type.h>
#include <vertex_setup.h>
#include <vulkan_buffer.h>

namespace vks {

class VulkanDevice;
class Model;
class ModelBuilder;

extern const eastl::string kBaseAssetsPath;
extern const eastl::string kBaseModelAssetsPath;

struct MeshesModelMatrices {
  VulkanBuffer buff;
  uint32_t num_meshes;
};

class ModelManager {
public:
  ModelManager();

  void LoadObjModel(const VulkanDevice &device, const eastl::string &filename,
                    const eastl::string &material_dir,
                    const VertexSetup &vertex_setup, Model **model) const;

  void LoadOtherModel(const VulkanDevice &device, const eastl::string &name,
                      const eastl::string &material_dir,
                      uint32_t assimp_post_process_steps,
                      const VertexSetup &vertex_setup, Model **model) const;

  void CreateModel(const VulkanDevice &device, const eastl::string &name,
                   const ModelBuilder &model_builder, Model **model) const;

  /**
   * @brief Create UBO array of all the model matrices, for each model
   *
   * @param device The device
   */
  void CreateModelMatrices(const VulkanDevice &device);

  void Shutdown(const VulkanDevice &device);

  /**
   * @brief Total number of meshes between all models.
   *
   * @return Number of total meshes
   */
  uint32_t GetMeshesCount() const;

  /**
   * @brief Return array UBO w/ matrices of all meshes of every model
   *
   * @param device The device
   */
  void GetMeshesModelMatricesBuffer(const VulkanDevice &device,
                                    MeshesModelMatrices &query) const;

  void set_deferred_gpass_set_layout(VkDescriptorSetLayout set_layout) {
    deferred_gpass_set_layout_ = set_layout;
  }

  void set_aniso_sampler(VkSampler aniso_sampler) {
    aniso_sampler_ = aniso_sampler;
  }

  void set_shade_material_name(const eastl::string &name) {
    shade_material_name_ = name;
  }

  void set_sets_desc_pool(VkDescriptorPool sets_desc_pool) {
    sets_desc_pool_ = sets_desc_pool;
  }

private:
  // List of all models
  typedef eastl::hash_map<eastl::string, eastl::unique_ptr<Model>> NameModelMap;
  mutable NameModelMap models_;
  VkDescriptorSetLayout deferred_gpass_set_layout_;
  VkSampler aniso_sampler_;
  eastl::string shade_material_name_;
  VkDescriptorPool sets_desc_pool_;

  void CreateUniqueModel(const VulkanDevice &device,
                         const ModelBuilder &init_info,
                         const eastl::string &name, Model **model) const;
}; // class ModelManager

} // namespace vks

#endif
