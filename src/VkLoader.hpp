#pragma once
#include <filesystem>
#include <unordered_map>
#include "Image.hpp"
#include "VkTypes.hpp"
#include "fastgltf/types.hpp"

struct GLTFMaterial
{
  MaterialInstance data;
};

struct Bounds
{
  glm::vec3 origin;
  float sphereRadius;
  glm::vec3 extents;
};

struct GeoSurface
{
  uint32_t startIndex;
  uint32_t count;
  Bounds bounds;
  std::shared_ptr<GLTFMaterial> material;
};

struct MeshAsset
{
  std::string name;

  std::vector<GeoSurface> surfaces;
  GPUMeshBuffers meshBuffers;
};

//forward declaration
class VkEngine;

struct LoadedGLTF : public IRenderable
{
  std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
  std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
  std::unordered_map<std::string, AllocatedImage> images;
  std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

  // nodes that dont have a parent, for iterating through the file in tree order
  std::vector<std::shared_ptr<Node>> topNodes;

  std::vector<VkSampler> samplers;

  DescriptorAllocatorGrowable descriptorPool;

  AllocatedBuffer materialDataBuffer;

  VkEngine* creator;

  ~LoadedGLTF()
  {
    clearAll();
  };

  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx);

 private:
  void clearAll();
};

namespace vkloader
{
  std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(VkEngine* engine, std::string_view filePath);
  // Legacy for debug
  std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VkEngine* engine, std::filesystem::path filePath);
  VkFilter extractFilter(fastgltf::Filter filter);
  VkSamplerMipmapMode extractMipmapMode(fastgltf::Filter filter);
  std::optional<AllocatedImage> loadImage(
    std::unique_ptr<VulkanBackend::Image>& image,
    VkEngine* engine,
    fastgltf::Asset& asset,
    fastgltf::Image& gltfImage);
}  // namespace vkloader
