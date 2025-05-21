#include "RaytracingPipeline.hpp"
#include "VkLoader.hpp"
#include "VkPipelines.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::RaytracingPipeline::RaytracingPipeline(
  std::unique_ptr<Device>& device,
  std::unique_ptr<PipelineLayout>& layout,
  std::string raygenPath,
  std::string missPath,
  std::string closestHitShader,
  std::string proceduralClosestHitShader,
  std::string proceduralIntersectionShader)
{
  // Load shaders.
  vkutil::loadShaderModule(raygenPath.c_str(), device->getHandle(), &_raygenShader);
  vkutil::loadShaderModule(missPath.c_str(), device->getHandle(), &_missShader);
  vkutil::loadShaderModule(closestHitShader.c_str(), device->getHandle(), &_closestHitShader);
  vkutil::loadShaderModule(proceduralClosestHitShader.c_str(), device->getHandle(), &_proceduralClosestHitShader);
  vkutil::loadShaderModule(proceduralIntersectionShader.c_str(), device->getHandle(), &_proceduralIntersectionShader);

  this->createShaderStages();
  this->createShaderGroups();

  VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  pipelineInfo.pNext = VK_NULL_HANDLE;
  pipelineInfo.flags = 0;
  pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
  pipelineInfo.pStages = _shaderStages.data();
  pipelineInfo.groupCount = static_cast<uint32_t>(_shaderGroups.size());
  pipelineInfo.pGroups = _shaderGroups.data();
  // The ray tracing process can shoot rays from the camera, and a shadow ray can be shot from the
  // hit points of the camera rays, hence a recursion level of 2. This number should be kept as low
  // as possible for performance reasons. Even recursive ray tracing should be flattened into a loop
  // in the ray generation to avoid deep recursion.
  pipelineInfo.maxPipelineRayRecursionDepth = 2;
  pipelineInfo.layout = layout->_handle;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = 0;

  auto createRayTracingPipelines =
    vkloader::loadFunction<PFN_vkCreateRayTracingPipelinesKHR>(device->getHandle(), "vkCreateRayTracingPipelinesKHR");
  VK_CHECK(createRayTracingPipelines(device->getHandle(), nullptr, nullptr, 1, &pipelineInfo, nullptr, &_handle));

  vkDestroyShaderModule(device->getHandle(), _raygenShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _missShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _closestHitShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _proceduralClosestHitShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _proceduralIntersectionShader, nullptr);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::RaytracingPipeline::createShaderStages()
{
  _shaderStages = {
    createShaderStageInfo(VK_SHADER_STAGE_RAYGEN_BIT_KHR, _raygenShader),
    createShaderStageInfo(VK_SHADER_STAGE_MISS_BIT_KHR, _missShader),
    createShaderStageInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _closestHitShader),
    createShaderStageInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _proceduralClosestHitShader),
    createShaderStageInfo(VK_SHADER_STAGE_INTERSECTION_BIT_KHR, _proceduralIntersectionShader)};
}

//--------------------------------------------------------------------------------------------------
VkPipelineShaderStageCreateInfo VulkanBackend::Raytracing::RaytracingPipeline::createShaderStageInfo(
  VkShaderStageFlagBits flag,
  VkShaderModule& module)
{
  VkPipelineShaderStageCreateInfo shaderStageInfo{};
  shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStageInfo.stage = flag;
  shaderStageInfo.module = module;
  shaderStageInfo.pName = "main";
  return shaderStageInfo;
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::RaytracingPipeline::createShaderGroups()
{
  // Shader groups
  VkRayTracingShaderGroupCreateInfoKHR rayGenGroupInfo = {};
  rayGenGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  rayGenGroupInfo.pNext = nullptr;
  rayGenGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  rayGenGroupInfo.generalShader = 0;
  rayGenGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
  rayGenGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  rayGenGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _raygenGroupIndex = 0;

  VkRayTracingShaderGroupCreateInfoKHR missGroupInfo = {};
  missGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  missGroupInfo.pNext = nullptr;
  missGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  missGroupInfo.generalShader = 1;
  missGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
  missGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  missGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _missGroupIndex = 1;

  VkRayTracingShaderGroupCreateInfoKHR triangleHitGroupInfo = {};
  triangleHitGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  triangleHitGroupInfo.pNext = nullptr;
  triangleHitGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  triangleHitGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
  triangleHitGroupInfo.closestHitShader = 2;
  triangleHitGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  triangleHitGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _triangleHitGroupIndex = 2;

  VkRayTracingShaderGroupCreateInfoKHR proceduralHitGroupInfo = {};
  proceduralHitGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  proceduralHitGroupInfo.pNext = nullptr;
  proceduralHitGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
  proceduralHitGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
  proceduralHitGroupInfo.closestHitShader = 3;
  proceduralHitGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  proceduralHitGroupInfo.intersectionShader = 4;
  _proceduralHitGroupIndex = 3;

  _shaderGroups = {rayGenGroupInfo, missGroupInfo, triangleHitGroupInfo, proceduralHitGroupInfo};
}
