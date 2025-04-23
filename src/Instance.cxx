#include "DebugUtils.hpp"
#include "Instance.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Instance::Instance(bool useValidationLayers)
{
  vkb::InstanceBuilder builder;
  vkb::Result builtInstance = builder.set_app_name("Vesuve Renderer")
                                .request_validation_layers(useValidationLayers)
                                .enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
                                .use_default_debug_messenger()
                                .require_api_version(1, 3, 0)
                                .build();
  if (!builtInstance)
  {
    // Log error and abort
    std::cerr << "Failed to create Vulkan instance: " << builtInstance.error().message() << std::endl;
  }
  else
  {
    _vkbHandle = builtInstance.value();
    _debugMessenger = _vkbHandle.debug_messenger;
  }
}
