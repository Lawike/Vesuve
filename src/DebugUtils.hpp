#include <iostream>
#include "VkLoader.hpp"
#include "VkTypes.hpp"

class DebugUtils
{
 public:
  DebugUtils() = default;
  ~DebugUtils() = default;

  static void SetObjectName(const VkAccelerationStructureKHR& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR);
  }
  static void SetObjectName(const VkBuffer& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_BUFFER);
  }
  static void SetObjectName(const VkCommandBuffer& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_COMMAND_BUFFER);
  }
  static void SetObjectName(const VkDescriptorSet& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_DESCRIPTOR_SET);
  }
  static void SetObjectName(const VkDescriptorSetLayout& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT);
  }
  static void SetObjectName(const VkDeviceMemory& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_DEVICE_MEMORY);
  }
  static void SetObjectName(const VkFramebuffer& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_FRAMEBUFFER);
  }
  static void SetObjectName(const VkImage& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_IMAGE);
  }
  static void SetObjectName(const VkImageView& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_IMAGE_VIEW);
  }
  static void SetObjectName(const VkPipeline& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_PIPELINE);
  }
  static void SetObjectName(const VkQueue& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_QUEUE);
  }
  static void SetObjectName(const VkRenderPass& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_RENDER_PASS);
  }
  static void SetObjectName(const VkSemaphore& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_SEMAPHORE);
  }
  static void SetObjectName(const VkShaderModule& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_SHADER_MODULE);
  }
  static void SetObjectName(const VkSwapchainKHR& object, const char* name, VkDevice device)
  {
    SetObjectName(object, name, device, VK_OBJECT_TYPE_SWAPCHAIN_KHR);
  }

  static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

  static void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

  static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

 private:
  template<typename T> static void SetObjectName(const T& object, const char* name, VkDevice device, VkObjectType type)
  {
#ifndef NDEBUG
    VkDebugUtilsObjectNameInfoEXT info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    info.pNext = nullptr;
    info.objectHandle = reinterpret_cast<const uint64_t&>(object);
    info.objectType = type;
    info.pObjectName = name;
    auto vkSetDebugUtilsObjectNameEXT =
      vkloader::loadFunction<PFN_vkSetDebugUtilsObjectNameEXT>(device, "vkSetDebugUtilsObjectNameEXT");
    VK_CHECK(vkSetDebugUtilsObjectNameEXT(device, &info));
#endif
  }
};
