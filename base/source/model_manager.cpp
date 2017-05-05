#include <model.h>
#include <model_manager.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <iostream>
#include <mesh.h>
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <utility>
#include <vector>
#define GLM_SWIZZLE_XYZW
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/vector3.h>
#include <base_system.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <logger.hpp>
#include <material.h>
#include <material_constants.h>
#include <material_instance.h>
#include <string>
#include <unordered_map>
#include <vulkan_tools.h>

namespace vks {

const eastl::string kBaseAssetsPath = "../assets/";
const eastl::string kBaseModelAssetsPath = "../assets/models/";

ModelManager::ModelManager()
    : models_(), deferred_gpass_set_layout_(VK_NULL_HANDLE) {}

void ModelManager::LoadObjModel(const VulkanDevice &device,
                                const eastl::string &filename,
                                const eastl::string &material_dir,
                                const VertexSetup &vertex_setup,
                                Model **model) const {
  if (SCAST_U32(models_.count(filename)) != 0U) {
    (*model) = models_[filename].get();
    return;
  }

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                              filename.c_str(), material_dir.c_str());

  if (!ret) {
    EXIT(err);
  }

  // Group all vertex data together
  std::unordered_map<Vertex, uint32_t> unique_vertices;
  uint32_t materials_count = SCAST_U32(materials.size());
  ModelBuilder model_builder(vertex_setup, sets_desc_pool_);

  // For each shape, which corresponds to a mesh in the model
  uint32_t shapes_size = SCAST_U32(shapes.size());
  eastl::vector<Mesh> meshes(shapes_size);
  for (uint32_t si = 0U; si < shapes_size; si++) {
    uint32_t index_offset = 0U;

    // Create a mesh
    meshes[si] = Mesh(SCAST_U32(model_builder.indices_data().size()),
                      SCAST_U32(shapes[si].mesh.indices.size()), 0U,
                      SCAST_U32(shapes[si].mesh.material_ids[0U]));

    // Load the vertices for this mesh
    uint32_t num_faces = SCAST_U32(shapes[si].mesh.num_face_vertices.size());
    for (uint32_t f = 0U; f < num_faces; f++) {
      uint32_t fv = shapes[si].mesh.num_face_vertices[f];

      for (uint32_t v = 0U; v < fv; v++) {
        tinyobj::index_t idx = shapes[si].mesh.indices[index_offset + v];
        Vertex vertex;
        vertex.pos = {attrib.vertices[3U * idx.vertex_index + 0U],
                      attrib.vertices[3U * idx.vertex_index + 1U],
                      attrib.vertices[3U * idx.vertex_index + 2U]};
        vertex.uv = {attrib.texcoords[2U * idx.texcoord_index + 0U],
                     attrib.texcoords[2U * idx.texcoord_index + 1U], 0.f};
        vertex.normal = {attrib.normals[3U * idx.normal_index + 0U],
                         attrib.normals[3U * idx.normal_index + 1U],
                         attrib.normals[3U * idx.normal_index + 2U]};

        if (unique_vertices.count(vertex) == 0U) {
          unique_vertices[vertex] = model_builder.current_vertex();
          model_builder.AddVertex(vertex);
        }

        model_builder.AddIndex(unique_vertices[vertex]);
      }

      index_offset += fv;
    }

    model_builder.AddMesh(&meshes[si]);
  }

  CreateUniqueModel(device, model_builder, filename, model);
  LOG("Meshes count: " << shapes_size);

  // Materials
  LOG("Materials count: " << materials_count);
  for (uint32_t i = 0U; i < materials_count; i++) {
    MaterialInstanceBuilder mat_builder(materials[i].name.c_str(), material_dir,
                                        aniso_sampler_);

    MaterialConstants mat_consts;
    mat_consts.emission =
        glm::vec3(materials[i].emission[0U], materials[i].emission[1U],
                  materials[i].emission[2U]);
    mat_consts.ambient =
        glm::vec3(materials[i].ambient[0U], materials[i].ambient[1U],
                  materials[i].ambient[2U]);
    mat_consts.diffuse_dissolve =
        glm::vec4(materials[i].diffuse[0U], materials[i].diffuse[1U],
                  materials[i].diffuse[2U], materials[i].dissolve);
    mat_consts.specular_shininess =
        glm::vec4(materials[i].specular[0U], materials[i].specular[1U],
                  materials[i].specular[2U], materials[i].shininess);
    mat_builder.AddConstants(mat_consts);

    MaterialBuilderTexture builder_texture;
    builder_texture.name = materials[i].ambient_texname.c_str();
    builder_texture.type = MatTextureType::AMBIENT;
    mat_builder.AddTexture(builder_texture);
    builder_texture.name = materials[i].diffuse_texname.c_str();
    builder_texture.type = MatTextureType::DIFFUSE;
    mat_builder.AddTexture(builder_texture);
    builder_texture.name = materials[i].specular_texname.c_str();
    builder_texture.type = MatTextureType::SPECULAR;
    mat_builder.AddTexture(builder_texture);
    builder_texture.name = materials[i].specular_highlight_texname.c_str();
    builder_texture.type = MatTextureType::SPECULAR_HIGHLIGHT;
    mat_builder.AddTexture(builder_texture);
    builder_texture.name = materials[i].bump_texname.c_str();
    builder_texture.type = MatTextureType::NORMAL;
    mat_builder.AddTexture(builder_texture);
    builder_texture.name = materials[i].alpha_texname.c_str();
    builder_texture.type = MatTextureType::ALPHA;
    mat_builder.AddTexture(builder_texture);
    builder_texture.name = materials[i].displacement_texname.c_str();
    builder_texture.type = MatTextureType::DISPLACEMENT;
    mat_builder.AddTexture(builder_texture);

    material_manager()->CreateMaterialInstance(device, mat_builder);
  }
}

void ModelManager::LoadOtherModel(const VulkanDevice &device,
                                  const eastl::string &filename,
                                  const eastl::string &material_dir,
                                  uint32_t assimp_post_process_steps,
                                  const VertexSetup &vertex_setup,
                                  Model **model) const {
  if (SCAST_U32(models_.count(filename)) != 0U) {
    (*model) = models_[filename].get();
    return;
  }

  Assimp::Importer assimp_importer;
  const aiScene *scene =
      assimp_importer.ReadFile(filename.c_str(), assimp_post_process_steps);

  if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE ||
      scene->mRootNode == nullptr) {
    EXIT(assimp_importer.GetErrorString());
  }

  uint32_t materials_count = scene->mNumMaterials;

  ModelBuilder model_builder(vertex_setup, sets_desc_pool_);

  uint32_t mat_idx_offset = material_manager()->GetMaterialInstancesCount();

  // Check the type of the loaded model; if it is OBJ, Assimp adds an
  // additional material at the beginning of the list of materials,
  // which also offset the index in the meshes
  uint32_t obj_offset = 0U;
  if (filename.find("obj") != eastl::string::npos) {
    obj_offset = 1U;
  }

  // For each shape, which corresponds to a mesh in the model
  uint32_t meshes_count = scene->mNumMeshes;
  std::vector<Mesh> meshes(meshes_count);
  uint32_t idx_offset = 0U;
  for (uint32_t mi = 0U; mi < meshes_count; mi++) {
    const aiMesh *ai_mesh = scene->mMeshes[mi];
    // Create a mesh
    meshes[mi] = Mesh(SCAST_U32(model_builder.indices_data().size()),
                      ai_mesh->mNumFaces * 3U, 0U,
                      ai_mesh->mMaterialIndex + mat_idx_offset - obj_offset);

    // Load the vertices for this mesh
    for (uint32_t i = 0U; i < ai_mesh->mNumVertices; i++) {
      Vertex vertex;
      vertex.pos = {ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y,
                    ai_mesh->mVertices[i].z};
      vertex.normal = {ai_mesh->mNormals[i].x, ai_mesh->mNormals[i].y,
                       ai_mesh->mNormals[i].z};
      if (ai_mesh->mTextureCoords[0U]) {
        vertex.uv = {ai_mesh->mTextureCoords[0U][i].x,
                     ai_mesh->mTextureCoords[0U][i].y,
                     ai_mesh->mTextureCoords[0U][i].z};
      } else {
        vertex.uv = {0.f, 0.f, 0.f};
      }
      if (assimp_post_process_steps & aiProcess_CalcTangentSpace) {
        vertex.bitangent = {ai_mesh->mBitangents[i].x,
                            ai_mesh->mBitangents[i].y,
                            ai_mesh->mBitangents[i].z};
        vertex.tangent = {ai_mesh->mTangents[i].x, ai_mesh->mTangents[i].y,
                          ai_mesh->mTangents[i].z};

        if (glm::dot(glm::cross(vertex.normal, vertex.tangent),
                     vertex.bitangent) < 0.0f) {
          vertex.tangent = vertex.tangent * -1.0f;
        }
      }
      model_builder.AddVertex(vertex);
    }

    // Load indices
    for (uint32_t i = 0U; i < ai_mesh->mNumFaces; i++) {
      aiFace &face = ai_mesh->mFaces[i];
      // Superfluous; all faces are triangulated
      for (uint32_t j = 0U; j < face.mNumIndices; j++) {
        model_builder.AddIndex(face.mIndices[j] + idx_offset);
        if (i == 0U || i == ai_mesh->mNumFaces - 1) {
        }
      }
    }

    idx_offset += ai_mesh->mNumVertices;
    model_builder.AddMesh(&meshes[mi]);
  }

  CreateUniqueModel(device, model_builder, filename, model);
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

    MaterialInstanceBuilder mat_builder(mat_name.C_Str(), material_dir,
                                        aniso_sampler_);

    MaterialConstants mat_consts;
    aiColor4D colour;
    if (ai_mat->Get(AI_MATKEY_COLOR_AMBIENT, colour) == AI_SUCCESS) {
      mat_consts.ambient = glm::vec3(colour.r, colour.g, colour.b);
    };
    if (ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, colour) == AI_SUCCESS) {
      mat_consts.diffuse_dissolve =
          glm::vec4(colour.r, colour.g, colour.b, 2.f);
    };
    if (ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, colour) == AI_SUCCESS) {
      mat_consts.specular_shininess =
          glm::vec4(colour.r, colour.g, colour.b, 10.f);
    };
    if (ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, colour) == AI_SUCCESS) {
      mat_consts.emission = glm::vec3(colour.r, colour.g, colour.b);
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
    if (ai_mat->GetTexture(aiTextureType_SHININESS, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    }
    mat_builder.AddTexture(builder_texture);

    builder_texture.type = MatTextureType::NORMAL;
    builder_texture.name = "";
    if (ai_mat->GetTexture(aiTextureType_NORMALS, 0U, &texture_path) ==
        AI_SUCCESS) {
      builder_texture.name = texture_path.C_Str();
    } else if (ai_mat->GetTexture(aiTextureType_HEIGHT, 0U, &texture_path) ==
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
}

void ModelManager::Shutdown(const VulkanDevice &device) {
  NameModelMap::iterator iter;
  for (iter = models_.begin(); iter != models_.end(); iter++) {
    iter->second->Shutdown(device);
  }
}

void ModelManager::GetMeshesModelMatricesBuffer(
    const VulkanDevice &device, MeshesModelMatrices &query) const {
  uint32_t num_meshes = GetMeshesCount();

  VulkanBufferInitInfo init_info;
  init_info.size = SCAST_U32(sizeof(glm::mat4)) * num_meshes;
  init_info.memory_property_flags = /*VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |*/
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  init_info.buffer_usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  // Create a large enough UBO of mat4s
  VulkanBuffer model_matrices_buff;
  model_matrices_buff.Init(device, init_info);

  // Upload the data to it
  uint32_t mat4_size = SCAST_U32(sizeof(glm::mat4));
  uint32_t counter = 0U;
  NameModelMap::iterator iter;
  for (iter = models_.begin(); iter != models_.end(); iter++, counter++) {
    uint32_t meshes_count = SCAST_U32(iter->second->meshes().size());
    for (uint32_t i = 0U; i < meshes_count; i++) {
      void *data = nullptr;
      model_matrices_buff.Map(device, &data, mat4_size, counter * mat4_size);
      *static_cast<glm::mat4 *>(data) = iter->second->meshes()[i].model_mat();
      model_matrices_buff.Unmap(device);
    }
  }

  query.buff = model_matrices_buff;
  query.num_meshes = num_meshes;
}

uint32_t ModelManager::GetMeshesCount() const {
  // Count meshes in model manager
  NameModelMap::const_iterator iter;
  uint32_t num_meshes = 0U;
  for (iter = models_.begin(); iter != models_.end(); iter++) {
    num_meshes += SCAST_U32(iter->second->meshes().size());
  }

  return num_meshes;
}

void ModelManager::CreateModel(const VulkanDevice &device,
                               const eastl::string &name,
                               const ModelBuilder &model_builder,
                               Model **model) const {
  CreateUniqueModel(device, model_builder, name, model);
}

void ModelManager::CreateUniqueModel(const VulkanDevice &device,
                                     const ModelBuilder &builder,
                                     const eastl::string &name,
                                     Model **model) const {
  if (SCAST_U32(models_.count(name)) != 0U) {
    (*model) = models_[name].get();
    return;
  }
  models_[name] = eastl::make_unique<Model>();
  models_[name]->Init(device, builder);
  (*model) = models_[name].get();
  LOG("Created model " + name + ".");
}

} // namespace vks
