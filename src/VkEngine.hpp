#pragma once

#include <VkBootstrap.h>
#include "Camera.hpp"
#include "ComputePipeline.hpp"
#include "DescriptorSet.hpp"
#include "DescriptorSetLayout.hpp"
#include "Device.hpp"
#include "FrameData.hpp"
#include "Image.hpp"
#include "Instance.hpp"
#include "Materials.hpp"
#include "PhysicalDevice.hpp"
#include "PointLight.hpp"
#include "RaytracingPipeline.hpp"
#include "ShaderBindingTable.hpp"
#include "Swapchain.hpp"
#include "TopLevelAccelerationStructure.hpp"
#include "VkDescriptors.hpp"
#include "VkLoader.hpp"
#include "VkTypes.hpp"
#include "Window.hpp"

using namespace VulkanBackend;
using namespace VulkanBackend::Raytracing;

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
  bool _isRaytracingEnabled{true};
  bool _isPreviousFrameRT{false};
  uint32_t maxNbOfFramesRT = 10;
  VkExtent2D _windowExtent{1700, 900};

  std::unique_ptr<Window> _window;
  std::unique_ptr<Instance> _instance;         // Vulkan library handle
  std::unique_ptr<PhysicalDevice> _chosenGPU;  // GPU chosen as the default device
  std::unique_ptr<Device> _device;             // Vulkan device for commands
  VkSurfaceKHR _surface;                       // Vulkan window surface
  VkQueue _presentQueue;                       // Vulkan present queue for presentation commands
  std::unique_ptr<Swapchain> _swapchain;       // Vulkan swapchain to transfer images between queues
  std::vector<std::unique_ptr<FrameData>> _frames;
  std::unique_ptr<FrameData>& getCurrentFrame()
  {
    return _frames.at(_frameNumber % FRAME_OVERLAP);
  }

  DeletionQueue _deletionQueue;  //Queue that keeps tracks of all the allocated structures.

  VmaAllocator _allocator;

  //draw resources
  std::unique_ptr<Image> _drawImage;
  VkExtent2D _drawExtent;
  float renderScale = 1.f;
  std::unique_ptr<Image> _depthImage;

  DescriptorAllocatorGrowable _globalDescriptorAllocator;

  std::unique_ptr<DescriptorSet> _drawImageDescriptors;
  std::unique_ptr<DescriptorSetLayout> _drawImageDescriptorLayout;

  // Background effects
  std::unique_ptr<ComputePipeline> _gradientPipeline;
  std::unique_ptr<PipelineLayout> _gradientPipelineLayout;
  std::unique_ptr<ComputePipeline> _skyPipeline;

  // immediate submit structures
  std::unique_ptr<Fence> _immFence;
  std::unique_ptr<CommandBuffer> _immCommandBuffer;
  std::unique_ptr<CommandPool> _immCommandPool;

  std::vector<ComputeEffect*> _backgroundEffects;
  int _currentBackgroundEffect{0};

  VkPipelineLayout _meshPipelineLayout;
  VkPipeline _meshPipeline;

  std::vector<std::shared_ptr<MeshAsset>> _testMeshes;

  bool _resize_requested = false;

  GPUSceneData _sceneData;

  std::unique_ptr<DescriptorSetLayout> _gpuSceneDataDescriptorLayout;
  std::unique_ptr<DescriptorSet> _gpuSceneDataDescriptorSet;

  //Default texture for tests
  std::unique_ptr<Image> _whiteImage;
  std::unique_ptr<Image> _blackImage;
  std::unique_ptr<Image> _greyImage;
  std::unique_ptr<Image> _errorCheckerboardImage;

  VkSampler _defaultSamplerLinear;
  VkSampler _defaultSamplerNearest;

  std::unique_ptr<DescriptorSetLayout> _singleImageDescriptorLayout;

  MaterialInstance _defaultData;
  GLTFMetallicRoughness _metalRoughMaterial;

  DrawContext _mainDrawContext;
  std::unordered_map<std::string, std::shared_ptr<Node>> _loadedNodes;
  std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> _loadedScenes;
  std::string _selectedNodeName = "Teapot";
  uint32_t _selectedMeshIndex = 0;
  std::string _selectedSceneName = "";

  Camera _mainCamera;

  EngineStats _stats;

  PointLight _mainLight;

  // Surface properties for tests
  SurfaceProperties _mainSurfaceProperties;

  // Raytracing
  std::unique_ptr<PipelineLayout> _raytracingPipelineLayout;
  std::unique_ptr<RaytracingPipeline> _raytracingPipeline;
  std::unique_ptr<Image> _accumulationImage;
  std::unique_ptr<RaytracingProperties> _raytracingProperties;
  std::unique_ptr<ShaderBindingTable> _shaderBindingTable;
  DescriptorAllocatorGrowable _raytracingDescriptorAllocator;
  std::unique_ptr<DescriptorSetLayout> _raytracingDescriptorSetLayout;
  std::unique_ptr<DescriptorSet> _raytracingDescriptorSet;
  std::vector<TopLevelAccelerationStructure> _topAS;
  std::vector<BottomLevelAccelerationStructure> _bottomAS;
  AllocatedBuffer _bottomBuffer;
  AllocatedBuffer _scratchBuffer;
  AllocatedBuffer _topBuffer;
  AllocatedBuffer _topScratchBuffer;
  AllocatedBuffer _instancesBuffer;

  static VkEngine& Get();

  // initializes everything in the engine
  void init();

  // shuts down the engine and cleans the allocated memory
  void cleanup();

  // draw loop
  void draw();
  void drawIndirect();
  void drawBackground(VkCommandBuffer cmd);
  void drawRaytracing(VkCommandBuffer cmd);
  void drawImgui(VkCommandBuffer cmd, VkImageView targetImageView);
  void drawGeometry(VkCommandBuffer cmd);
  void drawMain(VkCommandBuffer cmd);

  // run main loop
  void run();

  void immediateSubmit(std::function<void(VkCommandBuffer)>&& function);

  GPUMeshBuffers uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices);

  void updateScene();

  AllocatedBuffer createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
  template<class T> void copyBuffer(VkCommandBuffer cmd, AllocatedBuffer& dstBuffer, const std::vector<T>& content);
  void destroyBuffer(const AllocatedBuffer& buffer);

  void createImage(
    std::unique_ptr<Image>& image,
    void* data,
    VkExtent3D size,
    VkFormat format,
    VkImageUsageFlags usage,
    bool mipmapped = false);
  void destroyImage(const AllocatedImage& img);
  void resetFrame();

 private:
  void initVulkan();
  void initSwapchain();
  void initFrameData();
  void initImmediateCommands();
  void initDescriptors();
  void initRaytracingDescriptors();
  void updateRaytracingDescriptors();
  void initPipelines();
  void initBackgroundPipelines();
  void initRaytracingPipeline();
  void initShaderBindingTable();
  void initAccelerationStructures();
  void createBottomLevelStructures(VkCommandBuffer cmd);
  void createTopLevelStructures(VkCommandBuffer cmd);
  void initDefaultData();
  void initMainCamera();
  void initLight();
  void destroySwapchain();
  void resizeSwapchain();
  void createSurface();
  void createMemoryAllocator();
  void createDrawImage();
  void createDepthImage();
  void updateFrame();

  //Debug tools
  MeshAsset createTestTriangleMesh();
  MeshAsset createTestQuadMesh();
};
