#include "ComputePipeline.hpp"
#include "VkPipelines.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::ComputePipeline::ComputePipeline(
  std::unique_ptr<Device>& device,
  std::unique_ptr<PipelineLayout>& pipelineLayout,
  std::string shaderPath,
  std::string effectName)
{
  if (!vkutil::loadShaderModule(shaderPath.c_str(), device->getHandle(), &_shader))
  {
    fmt::println("Error when building the compute shader : ", shaderPath);
  }
  VkPipelineShaderStageCreateInfo stageinfo{};
  stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stageinfo.pNext = nullptr;
  stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  stageinfo.module = _shader;
  stageinfo.pName = "main";

  VkComputePipelineCreateInfo computePipelineCreateInfo{};
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.pNext = nullptr;
  computePipelineCreateInfo.layout = pipelineLayout->_handle;
  computePipelineCreateInfo.stage = stageinfo;

  VK_CHECK(vkCreateComputePipelines(device->getHandle(), VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &_handle));

  _effect.layout = pipelineLayout->_handle;
  _effect.name = effectName.c_str();
  _effect.pipeline = _handle;
  _effect.data = {};
}
