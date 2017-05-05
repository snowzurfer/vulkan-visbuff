#ifndef VKS_MESHESHEAPMANAGER
#define VKS_MESHESHEAPMANAGER

#include <EASTL/hash_map.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <assimp/postprocess.h>
#include <meshes_heap.h>
#include <vertex_setup.h>
#include <vulkan_buffer.h>

namespace vks {

extern const eastl::string kBaseAssetsPath;
extern const eastl::string kBaseModelAssetsPath;

class ModelWithHeaps {
public:
  void AddHeap(eastl::unique_ptr<MeshesHeap> heap);

  const eastl::vector<eastl::unique_ptr<MeshesHeap>> &heaps() const {
    return heaps_;
  }

  void CreateAndWriteDescriptorSets(VkDescriptorSetLayout heap_set_layout);

private:
  eastl::vector<eastl::unique_ptr<MeshesHeap>> heaps_;
}; // class ModelWithHeaps

class MeshesHeapManager {
public:
  void LoadOtherModel(const VulkanDevice &device, const eastl::string &name,
                      const eastl::string &material_dir,
                      uint32_t assimp_post_process_steps,
                      const VertexSetup &vertex_setup,
                      ModelWithHeaps **model) const;

  void set_aniso_sampler(VkSampler aniso_sampler) {
    aniso_sampler_ = aniso_sampler;
  }

  void set_shade_material_name(const eastl::string &name) {
    shade_material_name_ = name;
  }

  void set_heap_sets_desc_pool(VkDescriptorPool heap_sets_desc_pool) {
    heap_sets_desc_pool_ = heap_sets_desc_pool;
  }

  void set_heap_set_layout(VkDescriptorSetLayout heap_set_layout) {
    heap_set_layout_ = heap_set_layout;
  }

  void Shutdown(const VulkanDevice &device);

private:
  typedef eastl::hash_map<eastl::string, eastl::unique_ptr<ModelWithHeaps>>
      NameModelMap;
  mutable NameModelMap models_;
  VkSampler aniso_sampler_;
  eastl::string shade_material_name_;
  VkDescriptorPool heap_sets_desc_pool_;
  VkDescriptorSetLayout heap_set_layout_;

}; // class MeshesHeapManager

} // namespace vks

#endif