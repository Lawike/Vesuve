#include "PipelineLayout.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::PipelineLayout::PipelineLayout(
  std::unique_ptr<Device>& device,
  std::unique_ptr<DescriptorSetLayout>& descriptorLayout,
  std::vector<VkPushConstantRange>& pushConstants)
{
  VkPipelineLayoutCreateInfo layout{};
  layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout.pNext = nullptr;
  layout.pSetLayouts = &descriptorLayout->_handle;
  layout.setLayoutCount = 1;
  layout.pPushConstantRanges = pushConstants.data();
  layout.pushConstantRangeCount = pushConstants.size();


  VK_CHECK(vkCreatePipelineLayout(device->getHandle(), &layout, nullptr, &_handle));
}
