#include "DebugUtils.hpp"
#include "Instance.hpp"

//--------------------------------------------------------------------------------------------------
VulkanBackend::Instance::Instance(bool useValidationLayers)
{
  vkb::InstanceBuilder builder;
  _vkbHandle = builder.set_app_name("Vesuve Renderer")
                 .request_validation_layers(useValidationLayers)
                 .use_default_debug_messenger()
                 .require_api_version(1, 3, 0)
                 .build()
                 .value();
  _debugMessenger = _vkbHandle.debug_messenger;
}
