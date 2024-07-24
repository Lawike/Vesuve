#include "SwapChainSupportDetails.hpp"

//--------------------------------------------------------------------------------------------------
SwapChainSupportDetails::SwapChainSupportDetails(std::shared_ptr<VkSurfaceKHR> surface)
{
	this->surface = surface;
}

//--------------------------------------------------------------------------------------------------
void SwapChainSupportDetails::querySwapChainSupport(VkPhysicalDevice& device) {

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, *(this->surface), &(this->capabilities));

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, *(this->surface), &formatCount, nullptr);

	if (formatCount != 0) {
		this->formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, *(this->surface), &formatCount, this->formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, *(this->surface), &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		this->presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, *(this->surface), &presentModeCount, this->presentModes.data());
	}
}
