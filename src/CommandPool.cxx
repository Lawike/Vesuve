#include "CommandPool.hpp"
#include "VkInitializers.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::CommandPool::CommandPool(std::unique_ptr<Device>& device)
{
  //create a command pool for commands submitted to the graphics queue.
  //we also want the pool to allow for resetting of individual command buffers
  VkCommandPoolCreateInfo commandPoolInfo =
    vkinit::commandPoolCreateInfo(device->getGraphicsQueueFamily(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  VK_CHECK(vkCreateCommandPool(device->getHandle(), &commandPoolInfo, nullptr, &_handle));
}
