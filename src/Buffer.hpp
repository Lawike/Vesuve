#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class Buffer {
public:
	VkDeviceSize size;
	VkBufferUsageFlags usage;
	VkMemoryPropertyFlags properties;
	VkBuffer refBuffer{};
	VkDeviceMemory memory{};

	Buffer(VkPhysicalDevice physicalDevice, VkDevice device,VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void copy(Buffer srcBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue);

private:
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void createBuffer();

};