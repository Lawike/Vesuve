#pragma once
#include "Instance.hpp"
#include "VkTypes.hpp"

namespace VulkanBackend
{
  class PhysicalDevice
  {
   public:
    PhysicalDevice(std::unique_ptr<Instance>& instance, VkSurfaceKHR& surface);

    vkb::PhysicalDevice getHandle() const
    {
      return _vkbHandle;
    }
    ~PhysicalDevice() = default;
   private:
    vkb::PhysicalDevice _vkbHandle;
    VkPhysicalDeviceVulkan13Features _features13{};
    VkPhysicalDeviceVulkan12Features _features12{};
  };
}  // namespace VulkanBackend
