#pragma once

#include "DescriptorSetLayout.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "TopLevelAccelerationStructure.hpp"
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
    void writeImage(std::unique_ptr<Device>& device, std::unique_ptr<Image>& image, uint32_t binding = 0);
    void writeBuffer(std::unique_ptr<Device>& device, AllocatedBuffer& buffer, uint32_t binding, size_t size, size_t offset);
    void writeAccelerationStructure(
      std::unique_ptr<Device>& device,
      Raytracing::TopLevelAccelerationStructure& tlas,
      uint32_t binding,
      VkWriteDescriptorSetAccelerationStructureKHR descInfo);
    void destroyPools(std::unique_ptr<Device>& device);
    void updateSet(std::unique_ptr<Device>& device);
    void clear();
    VkDescriptorSet _handle;
    DescriptorAllocatorGrowable* _allocator;

   private:
    std::deque<VkDescriptorImageInfo> _imageInfos;
    std::deque<VkDescriptorBufferInfo> _bufferInfos;
    std::deque<VkWriteDescriptorSetAccelerationStructureKHR> _TLASInfos;
    std::vector<VkWriteDescriptorSet> _writes;
  };
}  // namespace VulkanBackend
