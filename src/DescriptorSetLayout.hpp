#pragma once
#include "Device.hpp"
#include "VkDescriptors.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class DescriptorSetLayout
  {
   public:
    DescriptorSetLayout(std::unique_ptr<Device>& device, std::vector<DescriptorBinding>& descriptorBindings);

    VkDescriptorSetLayout _handle;
  };
}  // namespace VulkanBackend
