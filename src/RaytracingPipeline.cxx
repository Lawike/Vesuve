#include "RaytracingPipeline.hpp"
#include "VkLoader.hpp"
#include "VkPipelines.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Raytracing::RaytracingPipeline::RaytracingPipeline(
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
  /** ,std::string proceduralClosestHitShader,
  std::string proceduralIntersectionShader*/)
{
  // Load shaders.
  vkutil::loadShaderModule(raygenPath.c_str(), device->getHandle(), &_raygenShader);
  vkutil::loadShaderModule(pathTraceRaygenPath.c_str(), device->getHandle(), &_pathTraceRaygenShader);
  vkutil::loadShaderModule(missPath.c_str(), device->getHandle(), &_missShader);
  vkutil::loadShaderModule(shadowMissPath.c_str(), device->getHandle(), &_shadowMissShader);
  vkutil::loadShaderModule(closestHitShader.c_str(), device->getHandle(), &_closestHitShader);
  vkutil::loadShaderModule(pathTraceClosestHitPath.c_str(), device->getHandle(), &_pathTraceClosestHitShader);
  vkutil::loadShaderModule(anyHitShader0.c_str(), device->getHandle(), &_anyHitShader0);
  vkutil::loadShaderModule(anyHitShader1.c_str(), device->getHandle(), &_anyHitShader1);
  //vkutil::loadShaderModule(proceduralClosestHitShader.c_str(), device->getHandle(), &_proceduralClosestHitShader);
  //vkutil::loadShaderModule(proceduralIntersectionShader.c_str(), device->getHandle(), &_proceduralIntersectionShader);

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
  _groupCount = _shaderGroups.size();
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
  vkDestroyShaderModule(device->getHandle(), _pathTraceRaygenShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _missShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _shadowMissShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _closestHitShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _pathTraceClosestHitShader, nullptr);
  vkDestroyShaderModule(device->getHandle(), _anyHitShader0, nullptr);
  vkDestroyShaderModule(device->getHandle(), _anyHitShader1, nullptr);
  //vkDestroyShaderModule(device->getHandle(), _proceduralClosestHitShader, nullptr);
  //vkDestroyShaderModule(device->getHandle(), _proceduralIntersectionShader, nullptr);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::Raytracing::RaytracingPipeline::createShaderStages()
{
  _shaderStages = {
    createShaderStageInfo(VK_SHADER_STAGE_RAYGEN_BIT_KHR, _raygenShader),
    createShaderStageInfo(VK_SHADER_STAGE_MISS_BIT_KHR, _missShader),
    createShaderStageInfo(VK_SHADER_STAGE_MISS_BIT_KHR, _shadowMissShader),
    createShaderStageInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _closestHitShader),
    createShaderStageInfo(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, _anyHitShader0),
    createShaderStageInfo(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, _anyHitShader1),
    createShaderStageInfo(VK_SHADER_STAGE_RAYGEN_BIT_KHR, _pathTraceRaygenShader),
    createShaderStageInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _pathTraceClosestHitShader)

    /** ,createShaderStageInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, _proceduralClosestHitShader),
    createShaderStageInfo(VK_SHADER_STAGE_INTERSECTION_BIT_KHR, _proceduralIntersectionShader)*/};
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

  VkRayTracingShaderGroupCreateInfoKHR shadowMissGroupInfo = {};
  shadowMissGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  shadowMissGroupInfo.pNext = nullptr;
  shadowMissGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  shadowMissGroupInfo.generalShader = 2;
  shadowMissGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
  shadowMissGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  shadowMissGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _shadowMissGroupIndex = 2;

  VkRayTracingShaderGroupCreateInfoKHR triangleHitGroupInfo = {};
  triangleHitGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  triangleHitGroupInfo.pNext = nullptr;
  triangleHitGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  triangleHitGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
  triangleHitGroupInfo.closestHitShader = 3;
  triangleHitGroupInfo.anyHitShader = 4;
  triangleHitGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _triangleHitGroupIndex = 3;

  // see https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/tree/master/ray_tracing_anyhit#fixing-the-pipeline
  VkRayTracingShaderGroupCreateInfoKHR anyHitGroupInfo = {};
  anyHitGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  anyHitGroupInfo.pNext = nullptr;
  anyHitGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  anyHitGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
  anyHitGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
  anyHitGroupInfo.anyHitShader = 5;
  anyHitGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _anyHitGroupIndex = 4;

  VkRayTracingShaderGroupCreateInfoKHR pathTraceRayGenGroupInfo = {};
  pathTraceRayGenGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  pathTraceRayGenGroupInfo.pNext = nullptr;
  pathTraceRayGenGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  pathTraceRayGenGroupInfo.generalShader = 6;
  pathTraceRayGenGroupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
  pathTraceRayGenGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  pathTraceRayGenGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _pathTraceRaygenGroupIndex = 5;

  VkRayTracingShaderGroupCreateInfoKHR pathTraceHitGroupInfo = {};
  pathTraceHitGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  pathTraceHitGroupInfo.pNext = nullptr;
  pathTraceHitGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  pathTraceHitGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
  pathTraceHitGroupInfo.closestHitShader = 7;
  pathTraceHitGroupInfo.anyHitShader = 4;
  pathTraceHitGroupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
  _pathTraceHitGroupIndex = 6;

  /* VkRayTracingShaderGroupCreateInfoKHR proceduralHitGroupInfo = {};
  proceduralHitGroupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
  proceduralHitGroupInfo.pNext = nullptr;
  proceduralHitGroupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
  proceduralHitGroupInfo.generalShader = VK_SHADER_UNUSED_KHR;
  proceduralHitGroupInfo.closestHitShader = 6;
  proceduralHitGroupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
  proceduralHitGroupInfo.intersectionShader = 7;
  _proceduralHitGroupIndex = 5;
  */
  _shaderGroups = {
    rayGenGroupInfo,
    missGroupInfo,
    shadowMissGroupInfo,
    triangleHitGroupInfo,
    anyHitGroupInfo,
    pathTraceRayGenGroupInfo,
    pathTraceHitGroupInfo,
    /* proceduralHitGroupInfo*/};
}
