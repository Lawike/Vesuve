#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif // !GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <chrono>
#include <vector>

#include "VesuveApp.hpp"
#include "DebugUtils.hpp"
#include "PhysicalDevicePicker.hpp"

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
//--------------------------------------------------------------------------------------------------
void VesuveApp::run()
{
	this->initWindow();
	this->initVulkan();
	this->mainLoop();
	this->cleanup();
}

//--------------------------------------------------------------------------------------------------
bool VesuveApp::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
std::vector<const char*> VesuveApp::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (this->enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

//--------------------------------------------------------------------------------------------------
void VesuveApp::setupDebugMessenger() {
	if (!this->enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	DebugUtils::populateDebugMessengerCreateInfo(createInfo);

	if (DebugUtils::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

//--------------------------------------------------------------------------------------------------
void VesuveApp::createInstance() {
	if (this->enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = this->getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (this->enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		DebugUtils::populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &(this->instance)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

//--------------------------------------------------------------------------------------------------
void VesuveApp::createSurface() {
	if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &(this->surface)) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

//--------------------------------------------------------------------------------------------------
void VesuveApp::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());
	std::shared_ptr<VkSurfaceKHR> surfacePtr = std::make_shared<VkSurfaceKHR>(surface);

	PhysicalDevicePicker picker{ surfacePtr };
	picker.pick(devices);
	physicalDevice = picker.pickedDevice;

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}
//--------------------------------------------------------------------------------------------------
void VesuveApp::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(this->windowWidth, this->windowHeight, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, this->framebufferResizeCallback);
}

//--------------------------------------------------------------------------------------------------
void VesuveApp::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<VesuveApp*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}

//--------------------------------------------------------------------------------------------------
void VesuveApp::initVulkan() {
	this->createInstance();
	this->setupDebugMessenger();
	this->createSurface();
	this->pickPhysicalDevice();
}
//--------------------------------------------------------------------------------------------------
void VesuveApp::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		//drawFrame();
	}

	vkDeviceWaitIdle(device);
}
//--------------------------------------------------------------------------------------------------
void VesuveApp::cleanup()
{
	vkDestroyDevice(device, nullptr);

	if (enableValidationLayers) {
		DebugUtils::DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}
