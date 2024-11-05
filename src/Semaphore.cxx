#include "Semaphore.hpp"
#include "VkInitializers.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Semaphore::Semaphore(std::unique_ptr<Device>& device)
{
  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphoreCreateInfo();
  VK_CHECK(vkCreateSemaphore(device->getHandle(), &semaphoreCreateInfo, nullptr, &_handle));
}
