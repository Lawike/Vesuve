#include <vector>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <vector>

class SwapChainSupportDetails {
public:
	SwapChainSupportDetails() = default;
	SwapChainSupportDetails(VkSurfaceKHR& surface);
	~SwapChainSupportDetails() = default;

	VkSurfaceCapabilitiesKHR capabilities = VkSurfaceCapabilitiesKHR{};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
	VkSurfaceKHR surface = VkSurfaceKHR{};

	void querySwapChainSupport(VkPhysicalDevice& device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat();
	VkPresentModeKHR chooseSwapPresentMode();
	VkExtent2D chooseSwapExtent(GLFWwindow* window);
};
