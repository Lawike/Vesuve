#include "DescriptorSetLayout.hpp"
#include "VkDescriptors.hpp"

VulkanBackend::DescriptorSetLayout::DescriptorSetLayout(
  std::unique_ptr<Device>& device,
  VkDescriptorType type,
  uint32_t flags)
{
  DescriptorLayoutBuilder builder;
  builder.addBinding(0, type);
  _handle = builder.build(device->getHandle(), flags);
}
