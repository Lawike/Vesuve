#pragma once

#include <VkBootstrap.h>
#include "Camera.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "Instance.hpp"
#include "Materials.hpp"
#include "PhysicalDevice.hpp"
#include "PointLight.hpp"
#include "Swapchain.hpp"
#include "VkDescriptors.hpp"
#include "VkLoader.hpp"
#include "VkTypes.hpp"
#include "Window.hpp"

using namespace VulkanBackend;

struct RenderObject
{
  uint32_t indexCount;
  uint32_t firstIndex;
  VkBuffer indexBuffer;

  MaterialInstance* material;

  glm::mat4 transform;
  VkDeviceAddress vertexBufferAddress;
  Bounds bounds;
};

struct DrawContext
{
  std::vector<RenderObject> OpaqueSurfaces;
  std::vector<RenderObject> TransparentSurfaces;
};

struct MeshNode : public Node
{
  std::shared_ptr<MeshAsset> mesh;

  virtual void Draw(const glm::mat4& topMatrix, DrawContext& ctx) override;
};

struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void push(std::function<void()>&& function)
  {
    deletors.push_back(function);
  }

  void flush()
  {
    // reverse iterate the deletion queue to execute all the functions
    for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
    {
      (*it)();  //call functors
    }

    deletors.clear();
  }
};

struct ComputePushConstants
{
  glm::vec4 data1;
  glm::vec4 data2;
  glm::vec4 data3;
  glm::vec4 data4;
};


struct FrameData
{
  VkCommandPool _commandPool;
  VkCommandBuffer _mainCommandBuffer;
  VkSemaphore _swapchainSemaphore, _renderSemaphore;
  VkFence _renderFence;
  DeletionQueue _deletionQueue;
  DescriptorAllocatorGrowable _frameDescriptors;
};

struct ComputeEffect
{
  const char* name;

  VkPipeline pipeline;
  VkPipelineLayout layout;

  ComputePushConstants data;
};

struct EngineStats
{
  float frametime;
  int triangleCount = 0;
  int drawcallCount = 0;
  float sceneUpdateTime;
  float meshDrawTime;
};

struct SurfaceProperties
{
  float specularCoefficient;
  float ambientCoefficient;
  float shininess;
  float screenGamma;
};


constexpr unsigned int FRAME_OVERLAP = 2;

class VkEngine
{
 public:
  VkEngine() = default;
  ~VkEngine();
  VkEngine(const VkEngine&) = delete;
  VkEngine(VkEngine&&) = delete;
  VkEngine& operator=(const VkEngine&) = delete;
  VkEngine& operator=(VkEngine&&) = delete;

  bool _isInitialized{false};
  int _frameNumber{0};
  bool _stopRendering{false};
  VkExtent2D _windowExtent{1700, 900};

  std::unique_ptr<Window> _window;
  std::unique_ptr<Instance> _instance;         // Vulkan library handle
  std::unique_ptr<PhysicalDevice> _chosenGPU;  // GPU chosen as the default device
  std::unique_ptr<Device> _device;             // Vulkan device for commands
  VkSurfaceKHR _surface;                       // Vulkan window surface
  VkQueue _presentQueue;                       // Vulkan present queue for presentation commands
  std::unique_ptr<Swapchain> _swapchain;       // Vulkan swapchain to transfer images between queues

  FrameData _frames[FRAME_OVERLAP];
  FrameData& getCurrentFrame()
  {
    return _frames[_frameNumber % FRAME_OVERLAP];
  };

  VkCommandPool _commandPool;

  DeletionQueue _deletionQueue;  //Queue that keeps tracks of all the allocated structures.

  VmaAllocator _allocator;

  //draw resources
  std::unique_ptr<Image> _drawImage;
  VkExtent2D _drawExtent;
  float renderScale = 1.f;
  std::unique_ptr<Image> _depthImage;

  DescriptorAllocatorGrowable _globalDescriptorAllocator;

  VkDescriptorSet _drawImageDescriptors;
  VkDescriptorSetLayout _drawImageDescriptorLayout;

  VkPipeline _gradientPipeline;
  VkPipelineLayout _gradientPipelineLayout;

  // immediate submit structures
  VkFence _immFence;
  VkCommandBuffer _immCommandBuffer;
  VkCommandPool _immCommandPool;

  std::vector<ComputeEffect> _backgroundEffects;
  int _currentBackgroundEffect{0};

  VkPipelineLayout _meshPipelineLayout;
  VkPipeline _meshPipeline;

  std::vector<std::shared_ptr<MeshAsset>> _testMeshes;

  bool resize_requested = false;

  GPUSceneData _sceneData;

  VkDescriptorSetLayout _gpuSceneDataDescriptorLayout;

  //Default texture for tests
  AllocatedImage _whiteImage;
  AllocatedImage _blackImage;
  AllocatedImage _greyImage;
  AllocatedImage _errorCheckerboardImage;

  VkSampler _defaultSamplerLinear;
  VkSampler _defaultSamplerNearest;

  VkDescriptorSetLayout _singleImageDescriptorLayout;

  MaterialInstance _defaultData;
  GLTFMetallicRoughness _metalRoughMaterial;

  DrawContext _mainDrawContext;
  std::unordered_map<std::string, std::shared_ptr<Node>> _loadedNodes;
  std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;
  std::string _selectedNodeName = "Teapot";
  std::string _selectedSceneName = "";

  Camera _mainCamera;

  EngineStats _stats;

  PointLight _mainLight;

  // Surface properties for tests
  SurfaceProperties _mainSurfaceProperties;

  static VkEngine& Get();

  // initializes everything in the engine
  void init();

  // shuts down the engine and cleans the allocated memory
  void cleanup();

  // draw loop
  void draw();
  void drawIndirect();
  void drawBackground(VkCommandBuffer cmd);
  void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView);
  void drawGeometry(VkCommandBuffer cmd);
  void drawMain(VkCommandBuffer cmd);

  // run main loop
  void run();

  void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

  GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

  void updateScene();

  AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
  void destroyBuffer(const AllocatedBuffer& buffer);

  AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
  AllocatedImage createImage(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false);
  void destroyImage(const AllocatedImage& img);

 private:
  void initVulkan();
  void initSwapchain();
  void initCommands();
  void initSyncStructures();
  void initDescriptors();
  void initPipelines();
  void initBackgroundPipelines();
  void initMeshPipeline();
  void initDefaultData();
  void initMainCamera();
  void initLight();
  void initImgui();
  void destroySwapchain();
  void resizeSwapchain();
  void createSurface();
  void createMemoryAllocator();

  void createDrawImage();
  void createDrawImageView();
  void createDepthImage();
  void createDepthImageView();
};
