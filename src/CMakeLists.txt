add_executable(Vesuve "main.cxx" "VesuveApp.cxx" "Vertex.cxx" "Vertex.hpp" "VesuveApp.hpp" "DebugUtils.hpp" "DebugUtils.cxx" "PhysicalDevicePicker.hpp" "PhysicalDevicePicker.cxx" "SwapChainSupportDetails.hpp" "SwapChainSupportDetails.cxx" "QueueFamilyIndices.cxx" "QueueFamilyIndices.hpp")

target_include_directories(Vesuve PRIVATE ${STB_DIR})
target_include_directories(Vesuve PRIVATE ${TINY_OBJ_LOADER_DIR})

target_link_libraries(Vesuve PRIVATE 
	glm::glm
	Vulkan::Vulkan
	glfw
)
