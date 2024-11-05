#pragma once

#include "DescriptorSetLayout.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "VkDescriptors.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class DescriptorSet
  {
   public:
    DescriptorSet(
      std::unique_ptr<Device>& device,
      std::unique_ptr<DescriptorSetLayout>& layout,
      DescriptorAllocatorGrowable& allocator);
    void writeImage(std::unique_ptr<Device>& device, std::unique_ptr<Image>& image);
    void destroyPools(std::unique_ptr<Device>& device);

    VkDescriptorSet _handle;
    DescriptorAllocatorGrowable* _allocator;
  };
}  // namespace VulkanBackend
