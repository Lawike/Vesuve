#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

class SingleTimeCommand {
public:
	VkCommandBuffer buffer{};
	VkCommandPool commandPool;
	VkDevice device;
	VkQueue graphicsQueue;

	SingleTimeCommand(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue);

	void begin();
	void end();
};
