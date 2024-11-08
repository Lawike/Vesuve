#pragma once
#include "PipelineLayout.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  namespace Raytracing
  {
    class RaytracingPipeline
    {
     public:
      RaytracingPipeline(
        std::unique_ptr<Device>& device,
        std::unique_ptr<PipelineLayout>& layout,
        std::string raygenPath,
        std::string missPath,
        std::string closestHitShader,
        std::string proceduralClosestHitShader,
        std::string proceduralIntersectionShader);
      VkPipeline _handle;

     private:
      void createShaderStages();
      VkPipelineShaderStageCreateInfo createShaderStageInfo(VkShaderStageFlagBits flag, VkShaderModule& module);
      void createShaderGroups();
      std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
      std::vector<VkRayTracingShaderGroupCreateInfoKHR> _shaderGroups;
      VkShaderModule _raygenShader;
      VkShaderModule _missShader;
      VkShaderModule _closestHitShader;
      VkShaderModule _proceduralClosestHitShader;
      VkShaderModule _proceduralIntersectionShader;
      uint32_t _raygenGroupIndex;
      uint32_t _missGroupIndex;
      uint32_t _triangleHitGroupIndex;
      uint32_t _proceduralHitGroupIndex;
    };
  }  // namespace Raytracing
}  // namespace VulkanBackend
