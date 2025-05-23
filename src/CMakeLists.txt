add_executable(
    Vesuve 
    "main.cxx"
    "VkEngine.cxx"
    "VkEngine.hpp"
    "DebugUtils.hpp"
    "DebugUtils.cxx"
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
    "UserInterface.hpp"
    "Instance.cxx"
    "Instance.hpp"
    "PhysicalDevice.cxx"
    "PhysicalDevice.hpp"
    "Device.cxx"
    "Device.hpp"
    "Swapchain.cxx"
    "Swapchain.hpp"
    "Image.cxx"
    "Image.hpp"
    "CommandPool.cxx"
    "CommandPool.hpp"
    "FrameData.cxx"
    "FrameData.hpp"
    "CommandBuffer.cxx"
    "CommandPool.hpp"
    "CommandBuffer.hpp"
    "Fence.cxx"
    "Fence.hpp"
    "Semaphore.cxx"
    "Semaphore.hpp"
    "DescriptorSetLayout.cxx"
    "DescriptorSetLayout.hpp"
    "DescriptorSet.cxx"
    "DescriptorSet.hpp"
    "PipelineLayout.cxx"
    "PipelineLayout.hpp"
    "ComputePipeline.cxx"
    "ComputePipeline.hpp"
    "RaytracingPipeline.cxx"
    "RaytracingPipeline.hpp"
    "ShaderBindingTable.cxx"
    "ShaderBindingTable.hpp"
    "RaytracingProperties.cxx"
    "RaytracingProperties.hpp"
   "AccelerationStructure.cxx" "AccelerationStructure.hpp" "TopLevelAccelerationStructure.hpp" "TopLevelAccelerationStructure.cxx" "BottomLevelAccelerationStructure.hpp" "BottomLevelAccelerationStructure.cxx"  "BottomLevelGeometry.hpp")

target_compile_definitions(Vesuve PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_link_libraries(Vesuve PRIVATE 
	glm::glm
	Vulkan::Vulkan
    fmt::fmt
    vk-bootstrap::vk-bootstrap
    imgui
    imgui_impl_vulkan
    imgui_impl_sdl2
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
    "*.rint"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.3 -V -g -o ${SPIRV} ${GLSL} 
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
