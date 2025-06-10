#pragma once
#include "VkDescriptors.hpp"
#include "VkTypes.hpp"

class GLTFMetallicRoughness
{
 public:
  MaterialPipeline _opaquePipeline;
  MaterialPipeline _transparentPipeline;

  VkDescriptorSetLayout _materialLayout;

  struct alignas(16) MaterialConstants
  {
    glm::vec4 colorFactors;
    glm::vec4 metalRoughFactors;
    glm::vec3 emissiveFactors;
    float emissivePower;
    float transparency;
    //padding, we need it anyway for uniform buffers (256 bytes alignement)
    float extra[51];
  };

  static_assert(sizeof(MaterialConstants) == 256);

  struct MaterialResources
  {
    AllocatedImage colorImage;
    VkSampler colorSampler;
    AllocatedImage metalRoughImage;
    VkSampler metalRoughSampler;
    VkBuffer dataBuffer;
    uint32_t dataBufferOffset;
  };

  DescriptorWriter writer;

  void buildPipelines(
    VkDevice device,
    VkDescriptorSetLayout gpuSceneDataDescriptorLayout,
    AllocatedImage drawImage,
    AllocatedImage depthImage);
  void clearResources(VkDevice device);

  MaterialInstance writeMaterial(
    VkDevice device,
    MaterialPass pass,
    const MaterialResources& resources,
    DescriptorAllocatorGrowable& descriptorAllocator);
};
