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
    void writeBuffer(std::unique_ptr<Device>& device, AllocatedBuffer& buffer, uint32_t binding, size_t offset);
    void
    writeBuffers(std::unique_ptr<Device>& device, std::vector<AllocatedBuffer>& buffer, uint32_t binding, size_t offset);
    void writeAccelerationStructure(
      std::unique_ptr<Device>& device,
      Raytracing::TopLevelAccelerationStructure& tlas,
      uint32_t binding,
      VkWriteDescriptorSetAccelerationStructureKHR descInfo);
    template<class TUniformData> void writeUniformBuffer(
      std::unique_ptr<Device>& device,
      AllocatedBuffer& buffer,
      TUniformData* destData,
      TUniformData& srcData,
      uint32_t binding,
      size_t size,
      size_t offset)
    {
      //write the buffer
      *destData = srcData;

      DescriptorWriter writer;
      writer.writeBuffer(binding, buffer.buffer, size, offset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
      writer.updateSet(device->getHandle(), this->_handle);
    }
    void destroyPools(std::unique_ptr<Device>& device);
    void updateSet(std::unique_ptr<Device>& device);
    void clear();
    VkDescriptorSet _handle;
    DescriptorAllocatorGrowable* _allocator;

   private:
    std::deque<VkDescriptorImageInfo> _imageInfos;
    std::vector<VkDescriptorBufferInfo> _bufferInfos;
    std::deque<VkWriteDescriptorSetAccelerationStructureKHR> _TLASInfos;
    std::vector<VkWriteDescriptorSet> _writes;
  };
}  // namespace VulkanBackend
