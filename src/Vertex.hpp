#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <glm/glm.hpp>
#include <array>

class Vertex {
public:
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	Vertex() = default;
	Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 texCoord);
	~Vertex() = default;

	/**
	* @returns the binding description object containing the corresponding stride of the Vertex data structure.
	*/
	static VkVertexInputBindingDescription getBindingDescription();

	/**
	* @returns the attribute description object containing the offset of each color component
	* , position & texture coordinates
	*/
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
	
	bool operator==(const Vertex& other) const;
};