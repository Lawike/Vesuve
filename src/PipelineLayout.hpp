#pragma once
#include "DescriptorSetLayout.hpp"

namespace VulkanBackend
{
  class PipelineLayout
  {
   public:
    PipelineLayout(
      std::unique_ptr<Device>& device,
      std::vector<VkDescriptorSetLayout>& descriptorLayout,
      std::vector<VkPushConstantRange>& pushConstants);

    VkPipelineLayout getHandle() const
    {
      return _handle;
    }

    VkPipelineLayout _handle;
  };
}  // namespace VulkanBackend
