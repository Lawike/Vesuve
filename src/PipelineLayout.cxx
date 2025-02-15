#include "PipelineLayout.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::PipelineLayout::PipelineLayout(
  std::unique_ptr<Device>& device,
  std::vector<VkDescriptorSetLayout>& descriptorLayout,
  std::vector<VkPushConstantRange>& pushConstants)
{
  VkPipelineLayoutCreateInfo layout{};
  layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout.pNext = nullptr;
  layout.pSetLayouts = &descriptorLayout.at(0);
  layout.setLayoutCount = descriptorLayout.size();
  layout.pPushConstantRanges = pushConstants.data();
  layout.pushConstantRangeCount = pushConstants.size();


  VK_CHECK(vkCreatePipelineLayout(device->getHandle(), &layout, nullptr, &_handle));
}
