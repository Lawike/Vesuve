#include "Buffer.hpp"
#include "MemoryTypeFinder.hpp"
#include "SingleTimeCommand.hpp"

#include <stdexcept>

//--------------------------------------------------------------------------------------------------
Buffer::Buffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
	this->physicalDevice = physicalDevice;
	this->device = device;
	this->size = size;
	this->usage = usage;
	this->properties = properties;

	this->createBuffer();
}

//--------------------------------------------------------------------------------------------------
void Buffer::copy(Buffer srcBuffer, VkDeviceSize size, VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue) {
	SingleTimeCommand command{ device, commandPool, graphicsQueue };
	command.begin();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(command.buffer, srcBuffer.refBuffer, this->refBuffer, 1, &copyRegion);

	command.end();
}

//--------------------------------------------------------------------------------------------------
void Buffer::createBuffer() {
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &refBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer !");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, refBuffer, &memRequirements);

	MemoryTypeFinder finder{ physicalDevice };

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = finder.findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory !");
	}

	vkBindBufferMemory(device, refBuffer, memory, 0);
}

