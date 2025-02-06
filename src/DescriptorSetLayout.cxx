#include "DescriptorSetLayout.hpp"
#include "VkDescriptors.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::DescriptorSetLayout::DescriptorSetLayout(
  std::unique_ptr<Device>& device,
  std::vector<DescriptorBinding>& descriptorBindings)
{
  DescriptorLayoutBuilder builder;
  for (const auto binding : descriptorBindings)
  {
    builder.addBinding(binding.binding, binding.type, binding.stage);
  }
  _handle = builder.build(device->getHandle());
}
