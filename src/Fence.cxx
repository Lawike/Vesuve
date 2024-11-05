#include "Fence.hpp"
#include "VkInitializers.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Fence::Fence(std::unique_ptr<Device>& device)
{
  //we want the fence to start signalled so we can wait on it on the first frame
  VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
  VK_CHECK(vkCreateFence(device->getHandle(), &fenceCreateInfo, nullptr, &_handle));
}
