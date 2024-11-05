#pragma once
#include "Device.hpp"
#include "PipelineLayout.hpp"

namespace VulkanBackend
{
  class ComputePipeline
  {
   public:
    ComputePipeline(
      std::unique_ptr<Device>& device,
      std::unique_ptr<PipelineLayout>& pipelineLayout,
      std::string shaderPath,
      std::string effectName);
    VkPipeline _handle;
    ComputeEffect _effect;
    VkShaderModule _shader;
  };

}  // namespace VulkanBackend
