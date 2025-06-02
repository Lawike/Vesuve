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
    float transparency;
    //padding, we need it anyway for uniform buffers (256 bytes alignement)
    glm::vec4 extra[13];
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

namespace VulkanBackend::Raytracing
{
  struct alignas(16) Material final
  {
    static Material Lambertian(const glm::vec3& diffuse, const int32_t textureId = -1, const float transparency = 0)
    {
      return Material{glm::vec4(diffuse, 1), textureId, 0.0f, 0.0f, transparency, Enum::Lambertian};
    }

    static Material
    Metallic(const glm::vec3& diffuse, const float fuzziness, const int32_t textureId = -1, const float transparency = 0)
    {
      return Material{glm::vec4(diffuse, 1), textureId, fuzziness, 0.0f, transparency, Enum::Metallic};
    }

    static Material Dielectric(const float refractionIndex, const int32_t textureId = -1, const float transparency = 0)
    {
      return Material{glm::vec4(0.7f, 0.7f, 1.0f, 1), textureId, 0.0f, refractionIndex, transparency, Enum::Dielectric};
    }

    static Material Isotropic(const glm::vec3& diffuse, const int32_t textureId = -1, const float transparency = 0)
    {
      return Material{glm::vec4(diffuse, 1), textureId, 0.0f, 0.0f, transparency, Enum::Isotropic};
    }

    static Material DiffuseLight(const glm::vec3& diffuse, const int32_t textureId = -1, const float transparency = 0)
    {
      return Material{glm::vec4(diffuse, 1), textureId, 0.0f, 0.0f, transparency, Enum::DiffuseLight};
    }

    enum class Enum : uint32_t
    {
      Lambertian = 0,
      Metallic = 1,
      Dielectric = 2,
      Isotropic = 3,
      DiffuseLight = 4
    };

    // Base material
    glm::vec4 Diffuse;
    int32_t DiffuseTextureId;

    // Metal fuzziness
    float Fuzziness;

    // Dielectric refraction index
    float RefractionIndex;

    // Which material are we dealing with
    float transparency;
    // Which material are we dealing with
    Enum MaterialModel;

    // Need to be a multiple of 16 because of
    float extra[3];
  };
  static_assert(sizeof(Material) % 16 == 0);

}  // namespace VulkanBackend::Raytracing
