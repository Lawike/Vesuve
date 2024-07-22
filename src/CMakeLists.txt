add_executable(Vesuve main.cxx "VesuveApp.cxx")

target_include_directories(Vesuve PRIVATE ${STB_DIR})
target_include_directories(Vesuve PRIVATE ${TINY_OBJ_LOADER_DIR})

target_link_libraries(Vesuve PRIVATE 
	glm::glm
	Vulkan::Vulkan
	glfw
)
