#include <vector>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>
#include <memory>

class SwapChainSupportDetails {
public:
	SwapChainSupportDetails(std::shared_ptr<VkSurfaceKHR> surface);

	VkSurfaceCapabilitiesKHR capabilities = VkSurfaceCapabilitiesKHR{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
	std::shared_ptr<VkSurfaceKHR> surface;

	void querySwapChainSupport(VkPhysicalDevice& device);
};
