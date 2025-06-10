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
      std::string pathTraceRaygenPath,
      std::string missPath,
      std::string shadowMissPath,
      std::string closestHitShader,
      std::string pathTraceClosestHitPath,
      std::string anyHitShader0,
      std::string anyHitShader1
        /** ,std::string proceduralClosestHitShader
        std::string proceduralIntersectionShader*/);
      VkPipeline getHandle() const
      {
        return _handle;
      }
      VkPipeline _handle;
      uint32_t _raygenGroupIndex;
      uint32_t _pathTraceRaygenGroupIndex;
      uint32_t _missGroupIndex;
      uint32_t _shadowMissGroupIndex;
      uint32_t _triangleHitGroupIndex;
      uint32_t _pathTraceHitGroupIndex;
      uint32_t _anyHitGroupIndex;
      uint32_t _proceduralHitGroupIndex;
      uint32_t _groupCount;

     private:
      void createShaderStages();
      VkPipelineShaderStageCreateInfo createShaderStageInfo(VkShaderStageFlagBits flag, VkShaderModule& module);
      void createShaderGroups();
      std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
      std::vector<VkRayTracingShaderGroupCreateInfoKHR> _shaderGroups;
      VkShaderModule _raygenShader;
      VkShaderModule _pathTraceRaygenShader;
      VkShaderModule _missShader;
      VkShaderModule _shadowMissShader;
      VkShaderModule _closestHitShader;
      VkShaderModule _pathTraceClosestHitShader;
      VkShaderModule _anyHitShader0;
      VkShaderModule _anyHitShader1;
      VkShaderModule _proceduralClosestHitShader;
      VkShaderModule _proceduralIntersectionShader;
    };
  }  // namespace Raytracing
}  // namespace VulkanBackend
