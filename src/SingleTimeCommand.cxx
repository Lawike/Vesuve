#include "SingleTimeCommand.hpp"

//--------------------------------------------------------------------------------------------------
SingleTimeCommand::SingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue)
{
  this->device = device;
  this->commandPool = commandPool;
  this->graphicsQueue = graphicsQueue;
}

//--------------------------------------------------------------------------------------------------
void SingleTimeCommand::begin()
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(device, &allocInfo, &buffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(buffer, &beginInfo);
}

void SingleTimeCommand::end()
{
  vkEndCommandBuffer(buffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &buffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  VK_CHECK(vkQueueWaitIdle(graphicsQueue));

  vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}
