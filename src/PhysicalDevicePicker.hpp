#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>
#include <memory>

class PhysicalDevicePicker {

public:
	PhysicalDevicePicker() = default;
	PhysicalDevicePicker(std::shared_ptr<VkSurfaceKHR> surface);
	~PhysicalDevicePicker() = default;

	std::shared_ptr<VkSurfaceKHR> surface;
	VkPhysicalDevice pickedDevice = nullptr;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	void pick(std::vector<VkPhysicalDevice> devices);
private:

	bool isDeviceSuitable(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice& device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice& device);

};