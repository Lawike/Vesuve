#include "DescriptorSet.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::DescriptorSet::DescriptorSet(
  std::unique_ptr<Device>& device,
  std::unique_ptr<DescriptorSetLayout>& layout,
  DescriptorAllocatorGrowable& allocator)
{
  _allocator = &allocator;
  _handle = _allocator->allocate(device->getHandle(), layout->_handle);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::writeImage(
  std::unique_ptr<Device>& device,
  std::unique_ptr<Image>& image,
  uint32_t binding)
{
  VkDescriptorImageInfo& info = _imageInfos.emplace_back(
    VkDescriptorImageInfo{
      .sampler = VK_NULL_HANDLE, .imageView = image->_handle.imageView, .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

  VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE;  //left empty for now until we need to write it
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  write.pImageInfo = &info;
  write.dstSet = _handle;

  _writes.push_back(write);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::writeBuffers(
  std::unique_ptr<Device>& device,
  std::vector<AllocatedBuffer>& buffers,
  uint32_t binding,
  size_t offset)
{
  for (const auto& buffer : buffers)
  {
    _bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer.buffer, .offset = offset, .range = VK_WHOLE_SIZE});
  }
  VkDescriptorBufferInfo& info = _bufferInfos.at(_bufferInfos.size() - buffers.size());

  VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE;  //left empty for now until we need to write it
  write.descriptorCount = buffers.size();
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.pBufferInfo = &info;
  write.dstSet = _handle;

  _writes.push_back(write);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::writeBuffer(
  std::unique_ptr<Device>& device,
  AllocatedBuffer& buffer,
  uint32_t binding,
  size_t offset)
{
  VkDescriptorBufferInfo info = VkDescriptorBufferInfo{.buffer = buffer.buffer, .offset = offset, .range = VK_WHOLE_SIZE};

  VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

  write.dstBinding = binding;
  write.dstSet = VK_NULL_HANDLE;  //left empty for now until we need to write it
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.pBufferInfo = &info;
  write.dstSet = _handle;

  _writes.push_back(write);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::writeAccelerationStructure(
  std::unique_ptr<Device>& device,
  Raytracing::TopLevelAccelerationStructure& tlas,
  uint32_t binding,
  VkWriteDescriptorSetAccelerationStructureKHR descInfo)
{
  VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstBinding = binding;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  write.dstArrayElement = 0;
  write.dstSet = _handle;
  write.pNext = &descInfo;

  _writes.push_back(write);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::destroyPools(std::unique_ptr<Device>& device)
{
  _allocator->destroyPools(device->getHandle());
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::updateSet(std::unique_ptr<Device>& device)
{
  vkUpdateDescriptorSets(device->getHandle(), (uint32_t)_writes.size(), _writes.data(), 0, nullptr);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::clear()
{
  _imageInfos.clear();
  _writes.clear();
  _bufferInfos.clear();
}
