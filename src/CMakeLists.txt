add_executable(
    Vesuve 
    "main.cxx"
    "VkEngine.cxx"
    "VkEngine.hpp"
    "DebugUtils.hpp"
    "DebugUtils.cxx"
    "PhysicalDevicePicker.hpp"
    "PhysicalDevicePicker.cxx"
    "SwapChainSupportDetails.hpp"
    "SwapChainSupportDetails.cxx"
    "QueueFamilyIndices.hpp"
    "QueueFamilyIndices.cxx"
    "Buffer.hpp"
    "Buffer.cxx"
    "MemoryTypeFinder.hpp"
    "MemoryTypeFinder.cxx"    
    "SingleTimeCommand.hpp"
    "SingleTimeCommand.cxx"
    "UniformBufferObject.hpp"
    "VkTypes.hpp"
    "VkInitializers.cxx"
    "VkInitializers.hpp"
    "VkImages.cxx"
    "VkImages.hpp"
    "VkDescriptors.cxx"
    "VkDescriptors.hpp"
    "VkPipelines.cxx"
    "VkPipelines.hpp"
    "VkLoader.hpp"
    "VkLoader.cxx"
    "Camera.cxx"
    "Camera.hpp"
    "Materials.hpp"
    "Materials.cxx"
    "PointLight.hpp"
    "Window.cxx"
    "Window.hpp"
    "UserInterface.cxx"
    "UserInterface.hpp")

target_compile_definitions(Vesuve PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

if(NOT DEFINED ENV{STB_DIR})
    message(FATAL_ERROR "La variable d'environnement STB_DIR n'est pas définie.")
endif()
target_include_directories(Vesuve PRIVATE $ENV{STB_DIR})

target_link_libraries(Vesuve PRIVATE 
	glm::glm
	Vulkan::Vulkan
    fmt::fmt
    vk-bootstrap::vk-bootstrap
    imgui::imgui
    imgui::imgui_impl_vulkan
    imgui::imgui_impl_sdl2
    fastgltf::fastgltf
)

# Custom target for shaders compilation
set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "*.frag"
    "*.vert"
    "*.comp"
    "*.rgen"
    "*.rmiss"
    "*.rchit"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)
add_dependencies(Vesuve Shaders)

add_custom_command(TARGET Vesuve POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Vesuve>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:Vesuve>/shaders"
)
