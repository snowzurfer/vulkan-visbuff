#include <meshes_heap_manager.h>
#include <vulkan_tools.h>
#define GLM_SWIZZLE_XYZW
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan_tools.h>
#include <logger.hpp>
#include <material.h>
#include <glm/glm.hpp>
#include <base_system.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/vector3.h>

namespace vks {

void ModelWithHeaps::AddHeap(eastl::unique_ptr<MeshesHeap> heap) {
  heaps_.push_back(eastl::move(heap));
  LOG("NUMMESHES: " << heaps_.back()->NumMeshes());
}

void MeshesHeapManager::LoadOtherModel(
    const VulkanDevice &device,
    const eastl::string &filename,
    const eastl::string &material_dir,
    uint32_t assimp_post_process_steps,
    const VertexSetup &vertex_setup,
    ModelWithHeaps **model) const {
  if (SCAST_U32(models_.count(filename)) != 0U) {
    (*model) = models_[filename].get();
    return;
  }

  Assimp::Importer assimp_importer;
  const aiScene *scene = assimp_importer.ReadFile(
      filename.c_str(),
      assimp_post_process_steps);
 
  if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE ||
      scene->mRootNode == nullptr) {
    EXIT(assimp_importer.GetErrorString());
  }

  uint32_t materials_count = scene->mNumMaterials;
 
  // Create a heaped model
  eastl::unique_ptr<ModelWithHeaps> current_model =
    eastl::make_unique<ModelWithHeaps>();
  
  eastl::unique_ptr<MeshesHeapBuilder> current_heap_builder =
    eastl::make_unique<MeshesHeapBuilder>(
        vertex_setup,
        heap_sets_desc_pool_);
  
  uint32_t mat_idx_offset = material_manager()->GetMaterialInstancesCount();

  // For each shape, which corresponds to a mesh in the model
  uint32_t meshes_count = scene->mNumMeshes;
  uint32_t idx_offset = 0U;
  for (uint32_t mi = 0U; mi < meshes_count; mi++) {
    const aiMesh *ai_mesh = scene->mMeshes[mi];

    // Try and fit mesh into current heap
    if (!current_heap_builder->TestMesh(ai_mesh->mNumVertices,
                                        ai_mesh->mNumFaces * 3U)) {
        // Create heap 
        current_model->AddHeap(
          eastl::make_unique<MeshesHeap>(device, *current_heap_builder.get()));

        // Change heap builder
        current_heap_builder.reset(nullptr);
        current_heap_builder =
          eastl::make_unique<MeshesHeapBuilder>(
              vertex_setup,
              heap_sets_desc_pool_);

        // Reset counters
        idx_offset = 0U;
    }

    // Create a mesh
    current_heap_builder->AddMesh(ai_mesh->mMaterialIndex + mat_idx_offset - 1U,
                                  ai_mesh->mNumFaces * 3U);

    //meshes[mi] = Mesh(
    //    SCAST_U32(model_builder.indices_data().size()),
    //    ai_mesh->mNumFaces * 3U,
    //    0U,
    //    ai_mesh->mMaterialIndex + mat_idx_offset - 1U);
   
    // Load the vertices for this mesh 
    for (uint32_t i = 0U; i < ai_mesh->mNumVertices; i++) {
      Vertex vertex; 
      vertex.pos = {
        ai_mesh->mVertices[i].x,
        ai_mesh->mVertices[i].y,
        ai_mesh->mVertices[i].z
      };
      vertex.normal = {
        ai_mesh->mNormals[i].x,
        ai_mesh->mNormals[i].y,
        ai_mesh->mNormals[i].z
      };
      if (ai_mesh->mTextureCoords[0U]) {
        vertex.uv = {
          ai_mesh->mTextureCoords[0U][i].x,
          ai_mesh->mTextureCoords[0U][i].y,
          ai_mesh->mTextureCoords[0U][i].z
        };
      }
      else {
        vertex.uv = { 0.f, 0.f, 0.f};
      }
      if (assimp_post_process_steps & aiProcess_CalcTangentSpace) {
        vertex.bitangent = {
          ai_mesh->mBitangents[i].x,
          ai_mesh->mBitangents[i].y,
          ai_mesh->mBitangents[i].z
        };
        vertex.tangent = {
          ai_mesh->mTangents[i].x,
          ai_mesh->mTangents[i].y,
          ai_mesh->mTangents[i].z
        };

        if (glm::dot(glm::cross(vertex.normal, vertex.tangent),
                     vertex.bitangent) < 0.0f) {
          vertex.tangent = vertex.tangent * -1.0f;
        }
      }
      
      current_heap_builder->AddVertex(vertex);
    }

    // Load indices
    for (uint32_t i = 0U; i < ai_mesh->mNumFaces; i++) {
      aiFace &face = ai_mesh->mFaces[i];
      // Superfluous; all faces are triangulated
      for (uint32_t j = 0U; j < face.mNumIndices; j++) {
        current_heap_builder->AddIndex(face.mIndices[j] + idx_offset);
      }
    }
    
    idx_offset += ai_mesh->mNumVertices;
  }
        
  // Create heap 
  current_model->AddHeap(
    eastl::make_unique<MeshesHeap>(device, *current_heap_builder.get()));

  //CreateUniqueModel(
  //    device,
  //    model_builder,
  //    filename,
  //    model);
  LOG("Meshes count: " << meshes_count);

  // Materials
  aiString assimp_default_mat_name("DefaultMaterial");
  LOG("Materials count: " << materials_count);
  for (uint32_t i = 0U; i < materials_count; i++) {
    // Avoid loading assimp's default material
    const aiMaterial *ai_mat = scene->mMaterials[i];

    aiString mat_name;
    ai_mat->Get(AI_MATKEY_NAME, mat_name);
    if (mat_name == assimp_default_mat_name) {
      continue;
    }

    MaterialInstanceBuilder mat_builder(
        mat_name.C_Str(),
        material_dir,
        aniso_sampler_);

    MaterialConstants mat_consts;
    aiColor4D colour;
    if (ai_mat->Get(AI_MATKEY_COLOR_AMBIENT, colour) == AI_SUCCESS) {
      mat_consts.ambient = glm::vec3(
          colour.r,
          colour.g,
          colour.b);
    };
    if (ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, colour) == AI_SUCCESS) {
      mat_consts.diffuse_dissolve = glm::vec4(
          colour.r,
          colour.g,
          colour.b,
          2.f);
    };
    if (ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, colour) == AI_SUCCESS) {
      mat_consts.specular_shininess = glm::vec4(
          colour.r,
          colour.g,
          colour.b,
          10.f);
    };
    if (ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, colour) == AI_SUCCESS) {
      mat_consts.emission = glm::vec3(
          colour.r,
          colour.g,
          colour.b);
    };
    float value = 0.f;
    if (ai_mat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
      mat_consts.specular_shininess.w = value;
    };
    if (ai_mat->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS) {
      mat_consts.diffuse_dissolve.w = value;
    };

    mat_builder.AddConstants(mat_consts);

    MaterialBuilderTexture builder_texture;
    aiString texture_path;
    builder_texture.type = MatTextureType::AMBIENT; 
    if (ai_mat->GetTexture(aiTextureType_AMBIENT, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);
   
    builder_texture.type = MatTextureType::DIFFUSE; 
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_DIFFUSE, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);
      
    builder_texture.type = MatTextureType::SPECULAR; 
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_SPECULAR, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);
    
    builder_texture.type = MatTextureType::SPECULAR_HIGHLIGHT; 
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_SHININESS , 0U,
                           &texture_path) == AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);
    
    builder_texture.type = MatTextureType::NORMAL; 
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_NORMALS, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    else if (ai_mat->GetTexture(aiTextureType_HEIGHT, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);
    
    builder_texture.type = MatTextureType::ALPHA; 
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_OPACITY, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);
    
    builder_texture.type = MatTextureType::DISPLACEMENT; 
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_DISPLACEMENT, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);

    material_manager()->CreateMaterialInstance(device, mat_builder);

  }

  models_[filename] = std::move(current_model);
  (*model) = models_[filename].get();
}
  
void ModelWithHeaps::CreateAndWriteDescriptorSets(
    VkDescriptorSetLayout heap_set_layout) {
  for (eastl::vector<eastl::unique_ptr<MeshesHeap>>::iterator itor =
         heaps_.begin();
       itor != heaps_.end();
       ++itor) {
    (*itor)->CreateAndWriteDescriptorSets(heap_set_layout);
  }
}
 

void MeshesHeapManager::Shutdown(const VulkanDevice &device) {
  models_.clear();
}

} // namespace vks
