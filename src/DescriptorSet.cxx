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
void VulkanBackend::DescriptorSet::writeImage(std::unique_ptr<Device>& device, std::unique_ptr<Image>& image)
{
  DescriptorWriter writer;
  writer.writeImage(0, image->_handle.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
  writer.updateSet(device->getHandle(), _handle);
}

//--------------------------------------------------------------------------------------------------
void VulkanBackend::DescriptorSet::destroyPools(std::unique_ptr<Device>& device)
{
  _allocator->destroyPools(device->getHandle());
}
