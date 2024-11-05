#pragma once
#include "Device.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class DescriptorSetLayout
  {
   public:
    DescriptorSetLayout(std::unique_ptr<Device>& device, VkDescriptorType type, uint32_t flags);

    VkDescriptorSetLayout _handle;
  };
}  // namespace VulkanBackend
