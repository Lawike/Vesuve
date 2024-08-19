#pragma once

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>

class PhysicalDevicePicker
{

 public:
  PhysicalDevicePicker() = default;
  PhysicalDevicePicker(VkSurfaceKHR& surface, std::vector<const char*>& deviceExtensions);
  ~PhysicalDevicePicker() = default;

  VkSurfaceKHR surface = VkSurfaceKHR{};
  VkPhysicalDevice pickedDevice = nullptr;
  VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
  std::vector<const char*> deviceExtensions;

  void pick(std::vector<VkPhysicalDevice> devices);
 private:
  bool isDeviceSuitable(VkPhysicalDevice device);
  VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice& device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice& device);
};
