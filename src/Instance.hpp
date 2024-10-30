#pragma once
#pragma once
#include "VkBootstrap.h"

namespace VulkanBackend
{
  class Instance
  {
   public:
    Instance(bool useValidationLayers);
    vkb::Instance getHandle() const
    {
      return _vkbHandle;
    }
    // Window instance properties.
    VkDebugUtilsMessengerEXT getDebugMessenger() const
    {
      return _debugMessenger;
    }
   private:
    vkb::Instance _vkbHandle;
    VkDebugUtilsMessengerEXT _debugMessenger;
  };

}  // namespace VulkanBackend
