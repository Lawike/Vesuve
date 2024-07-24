#include <vector>
#include "Vertex.hpp"

class VesuveApp {
public:
	void run();
	VesuveApp() = default;
	~VesuveApp() = default;

private:
	GLFWwindow* window;
	uint32_t windowWidth = 800;
	uint32_t windowHeight = 600;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;

	bool enableValidationLayers;

	// Devices
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// Queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	// Swap chains
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	// Renderpass
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	/*
	// Data structures
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	// Buffers
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	// Descriptors
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// Synchronization
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;
	*/
	// texture & mipmapping
	//uint32_t mipLevels;
	//VkImage textureImage;
	//VkDeviceMemory textureImageMemory;
	// VkImageView textureImageView;
	/*
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	*/
	bool framebufferResized = false;
	/*
	// Anti aliasing
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	// Display image
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	*/
	//Support
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();

	/**
	* Creates and initializes the application window using GLFW
	*/
	void initWindow();
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	void initVulkan();
	void createInstance();
	void setupDebugMessenger();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createRenderPass();
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createColorResources();
	void createDepthResources();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createFramebuffers();
	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();
	void mainLoop();

	void cleanup();
	void cleanupSwapChain();
};