#include "CommandBuffer.hpp"
#include "VkInitializers.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::CommandBuffer::CommandBuffer(std::unique_ptr<Device>& device, std::unique_ptr<CommandPool>& commandPool)
{
  // allocate the default command buffer that we will use for rendering
  VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::commandBufferAllocateInfo(commandPool->getHandle(), 1);

  VK_CHECK(vkAllocateCommandBuffers(device->getHandle(), &cmdAllocInfo, &_handle));
}
