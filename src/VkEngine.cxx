#define VMA_IMPLEMENTATION
#ifndef IMGUI_H
#define IMGUI_H
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#endif
#ifndef SDL_H
#define SDL_H
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#endif
#include <vma/vk_mem_alloc.h>
#include <chrono>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/packing.hpp>
#include <set>
#include <thread>
#include "DebugUtils.hpp"
#include "PipelineLayout.hpp"
#include "UserInterface.hpp"
#include "VkDescriptors.hpp"
#include "VkEngine.hpp"
#include "VkImages.hpp"
#include "VkInitializers.hpp"
#include "VkPipelines.hpp"

constexpr bool bUseValidationLayers = true;

VkEngine* loadedEngine = nullptr;

const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};

bool is_visible(const RenderObject& obj, const glm::mat4& viewproj)
{
  std::array<glm::vec3, 8> corners{
    glm::vec3{1, 1, 1},
    glm::vec3{1, 1, -1},
    glm::vec3{1, -1, 1},
    glm::vec3{1, -1, -1},
    glm::vec3{-1, 1, 1},
    glm::vec3{-1, 1, -1},
    glm::vec3{-1, -1, 1},
    glm::vec3{-1, -1, -1},
  };

  glm::mat4 matrix = viewproj * obj.transform;

  glm::vec3 min = {1.5, 1.5, 1.5};
  glm::vec3 max = {-1.5, -1.5, -1.5};

  for (int c = 0; c < 8; c++)
  {
    // project each corner into clip space
    glm::vec4 v = matrix * glm::vec4(obj.bounds.origin + (corners[c] * obj.bounds.extents), 1.f);

    // perspective correction
    v.x = v.x / v.w;
    v.y = v.y / v.w;
    v.z = v.z / v.w;

    min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
    max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
  }

  // check the clip space box is within the view
  if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
  {
    return false;
  }
  else
  {
    return true;
  }
}


//--------------------------------------------------------------------------------------------------
VkEngine& VkEngine::Get()
{
  return *loadedEngine;
}

//--------------------------------------------------------------------------------------------------
VkEngine::~VkEngine()
{
}

//--------------------------------------------------------------------------------------------------
void VkEngine::init()
{
  // We initialize SDL and create a window with it.
  _window = std::make_unique<Window>(_windowExtent);

  this->initVulkan();
  this->initSwapchain();
  this->initFrameData();
  this->initImmediateCommands();
  this->initDescriptors();
  this->initPipelines();
  UserInterface::init(this);
  this->initDefaultData();
  this->initMainCamera();
  this->initLight();

  //everything went fine
  _isInitialized = true;

  std::string structurePath = {"../assets/structure.glb"};
  /*auto structureFile = vkloader::loadGltf(this, structurePath);

  assert(structureFile.has_value());

  _loadedScenes["structure"] = *structureFile;
  */
}

//--------------------------------------------------------------------------------------------------
void VkEngine::cleanup()
{
  if (_isInitialized)
  {
    //make sure the gpu has stopped doing its things
    vkDeviceWaitIdle(_device->getHandle());

    _loadedScenes.clear();

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
      //already written from before
      vkDestroyCommandPool(_device->getHandle(), _frames[i]->_commandPool->getHandle(), nullptr);

      //destroy sync objects
      vkDestroyFence(_device->getHandle(), _frames[i]->_renderFence->_handle, nullptr);
      vkDestroySemaphore(_device->getHandle(), _frames[i]->_renderSemaphore->_handle, nullptr);
      vkDestroySemaphore(_device->getHandle(), _frames[i]->_swapchainSemaphore->_handle, nullptr);

      _frames[i]->_deletionQueue.flush();
    }

    for (auto& mesh : _testMeshes)
    {
      destroyBuffer(mesh->meshBuffers.indexBuffer);
      destroyBuffer(mesh->meshBuffers.vertexBuffer);
    }

    _metalRoughMaterial.clearResources(_device->getHandle());

    _deletionQueue.flush();

    this->destroySwapchain();

    vkDestroySurfaceKHR(_instance->getHandle(), _surface, nullptr);

    vkDestroyDevice(_device->getHandle(), nullptr);
    if (bUseValidationLayers)
    {
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        _instance->getHandle(), "vkDestroyDebugUtilsMessengerEXT");
      func(_instance->getHandle(), _instance->getDebugMessenger(), nullptr);
    }
    vkDestroyInstance(_instance->getHandle(), nullptr);
  }
  // clear engine pointer
  loadedEngine = nullptr;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::draw()
{
  //wait until the gpu has finished rendering the last frame. Timeout of 1 second
  VK_CHECK(vkWaitForFences(_device->getHandle(), 1, &this->getCurrentFrame()->_renderFence->_handle, true, 1000000000));

  this->getCurrentFrame()->_deletionQueue.flush();
  this->getCurrentFrame()->_frameDescriptors.clearPools(_device->getHandle());
  //request image from the swapchain
  uint32_t swapchainImageIndex;

  VkResult e = vkAcquireNextImageKHR(
    _device->getHandle(),
    _swapchain->getHandle(),
    1000000000,
    this->getCurrentFrame()->_swapchainSemaphore->_handle,
    nullptr,
    &swapchainImageIndex);
  if (e == VK_ERROR_OUT_OF_DATE_KHR)
  {
    resize_requested = true;
    return;
  }

  _drawExtent.height =
    std::min(_swapchain->getSwapchainExtent().height, _drawImage->_handle.imageExtent.height) * renderScale;
  _drawExtent.width = std::min(_swapchain->getSwapchainExtent().width, _drawImage->_handle.imageExtent.width) * renderScale;

  VK_CHECK(vkResetFences(_device->getHandle(), 1, &this->getCurrentFrame()->_renderFence->_handle));

  //now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
  VK_CHECK(vkResetCommandBuffer(this->getCurrentFrame()->_mainCommandBuffer->getHandle(), 0));

  //naming it cmd for shorter writing
  VkCommandBuffer cmd = this->getCurrentFrame()->_mainCommandBuffer->getHandle();

  //begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  // transition our main draw image into general layout so we can write into it
  // we will overwrite it all so we dont care about what was the older layout
  vkutil::transitionImage(cmd, _drawImage->_handle.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
  vkutil::transitionImage(
    cmd, _depthImage->_handle.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  drawMain(cmd);

  //transtion the draw image and the swapchain image into their correct transfer layouts
  vkutil::transitionImage(cmd, _drawImage->_handle.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
  vkutil::transitionImage(
    cmd,
    _swapchain->getSwapchainImages()[swapchainImageIndex],
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkExtent2D extent;
  extent.height = _windowExtent.height;
  extent.width = _windowExtent.width;

  // execute a copy from the draw image into the swapchain
  vkutil::copyImageToImage(
    cmd,
    _drawImage->_handle.image,
    _swapchain->getSwapchainImages()[swapchainImageIndex],
    _drawExtent,
    _swapchain->getSwapchainExtent());

  // set swapchain image layout to Attachment Optimal so we can draw it
  vkutil::transitionImage(
    cmd,
    _swapchain->getSwapchainImages()[swapchainImageIndex],
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

  //draw imgui into the swapchain image
  drawImgui(cmd, _swapchain->getSwapchainImageViews()[swapchainImageIndex]);

  // set swapchain image layout to Present so we can draw it
  vkutil::transitionImage(
    cmd,
    _swapchain->getSwapchainImages()[swapchainImageIndex],
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  //finalize the command buffer (we can no longer add commands, but it can now be executed)
  VK_CHECK(vkEndCommandBuffer(cmd));

  //prepare the submission to the queue.
  //we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
  //we will signal the _renderSemaphore, to signal that rendering has finished

  VkCommandBufferSubmitInfo cmdinfo = vkinit::commandBufferSubmitInfo(cmd);

  VkSemaphoreSubmitInfo waitInfo = vkinit::semaphoreSubmitInfo(
    VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, this->getCurrentFrame()->_swapchainSemaphore->_handle);
  VkSemaphoreSubmitInfo signalInfo =
    vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, this->getCurrentFrame()->_renderSemaphore->_handle);

  VkSubmitInfo2 submit = vkinit::submitInfo(&cmdinfo, &signalInfo, &waitInfo);

  //submit command buffer to the queue and execute it.
  // _renderFence will now block until the graphic commands finish execdution
  VK_CHECK(vkQueueSubmit2(_device->getGraphicsQueue(), 1, &submit, this->getCurrentFrame()->_renderFence->_handle));


  //prepare present
  // this will put the image we just rendered to into the visible window.
  // we want to wait on the _renderSemaphore for that,
  // as its necessary that drawing commands have finished before the image is displayed to the user
  VkPresentInfoKHR presentInfo = vkinit::presentInfo();
  presentInfo.pSwapchains = &(_swapchain->_vkbHandle.swapchain);
  presentInfo.swapchainCount = 1;

  presentInfo.pWaitSemaphores = &(this->getCurrentFrame()->_renderSemaphore->_handle);
  presentInfo.waitSemaphoreCount = 1;

  presentInfo.pImageIndices = &swapchainImageIndex;

  VkResult presentResult = vkQueuePresentKHR(_device->getGraphicsQueue(), &presentInfo);
  if (e == VK_ERROR_OUT_OF_DATE_KHR)
  {
    resize_requested = true;
    return;
  }
  //increase the number of frames drawn
  _frameNumber++;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::drawIndirect()
{
}

//--------------------------------------------------------------------------------------------------
void VkEngine::drawBackground(VkCommandBuffer cmd)
{
  //make a clear-color from frame number. This will flash with a 120 frame period.
  VkClearColorValue clearValue;
  float flash = std::abs(std::sin(_frameNumber / 120.f));
  clearValue = {{0.0f, 0.0f, flash, 1.0f}};

  VkImageSubresourceRange clearRange = vkinit::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);

  ComputeEffect& effect = *_backgroundEffects[_currentBackgroundEffect];

  // bind the background compute pipeline
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(
    cmd,
    VK_PIPELINE_BIND_POINT_COMPUTE,
    _gradientPipelineLayout->_handle,
    0,
    1,
    &_drawImageDescriptors->_handle,
    0,
    nullptr);

  vkCmdPushConstants(
    cmd, _gradientPipelineLayout->_handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
  vkCmdDispatch(cmd, std::ceil(_drawExtent.width / 16.0), std::ceil(_drawExtent.height / 16.0), 1);
}

//--------------------------------------------------------------------------------------------------
void VkEngine::drawImgui(VkCommandBuffer cmd, VkImageView targetImageView)
{
  VkRenderingAttachmentInfo colorAttachment =
    vkinit::attachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  VkRenderingInfo renderInfo = vkinit::renderingInfo(_swapchain->getSwapchainExtent(), &colorAttachment, nullptr);

  vkCmdBeginRendering(cmd, &renderInfo);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

  vkCmdEndRendering(cmd);
}

//--------------------------------------------------------------------------------------------------
void VkEngine::drawGeometry(VkCommandBuffer cmd)
{
  std::vector<uint32_t> opaqueDraws;
  opaqueDraws.reserve(_mainDrawContext.OpaqueSurfaces.size());

  for (int i = 0; i < _mainDrawContext.OpaqueSurfaces.size(); i++)
  {
    if (is_visible(_mainDrawContext.OpaqueSurfaces[i], _sceneData.viewproj))
    {
      opaqueDraws.push_back(i);
    }
  }

  // sort the opaque surfaces by material and mesh
  std::sort(
    opaqueDraws.begin(),
    opaqueDraws.end(),
    [&](const auto& iA, const auto& iB)
    {
      const RenderObject& A = _mainDrawContext.OpaqueSurfaces[iA];
      const RenderObject& B = _mainDrawContext.OpaqueSurfaces[iB];
      if (A.material == B.material)
      {
        return A.indexBuffer < B.indexBuffer;
      }
      else
      {
        return A.material < B.material;
      }
    });

  //allocate a new uniform buffer for the scene data
  AllocatedBuffer gpuSceneDataBuffer =
    this->createBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  //add it to the deletion queue of this frame so it gets deleted once its been used
  this->getCurrentFrame()->_deletionQueue.push([=, this]() { this->destroyBuffer(gpuSceneDataBuffer); });

  //write the buffer
  GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();
  *sceneUniformData = _sceneData;

  //create a descriptor set that binds that buffer and update it
  VkDescriptorSet globalDescriptor =
    this->getCurrentFrame()->_frameDescriptors.allocate(_device->getHandle(), _gpuSceneDataDescriptorLayout->_handle);

  DescriptorWriter writer;
  writer.writeBuffer(0, gpuSceneDataBuffer.buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.updateSet(_device->getHandle(), globalDescriptor);

  //defined outside of the draw function, this is the state we will try to skip
  MaterialPipeline* lastPipeline = nullptr;
  MaterialInstance* lastMaterial = nullptr;
  VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

  auto draw = [&](const RenderObject& r)
  {
    if (r.material != lastMaterial)
    {
      lastMaterial = r.material;
      //rebind pipeline and descriptors if the material changed
      if (r.material->pipeline != lastPipeline)
      {
        lastPipeline = r.material->pipeline;
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
        vkCmdBindDescriptorSets(
          cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1, &globalDescriptor, 0, nullptr);

        VkViewport viewport = {};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)_windowExtent.width;
        viewport.height = (float)_windowExtent.height;
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0;
        scissor.offset.y = 0;
        scissor.extent.width = _windowExtent.width;
        scissor.extent.height = _windowExtent.height;

        vkCmdSetScissor(cmd, 0, 1, &scissor);
      }

      vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1, &r.material->materialSet, 0, nullptr);
    }
    //rebind index buffer if needed
    if (r.indexBuffer != lastIndexBuffer)
    {
      lastIndexBuffer = r.indexBuffer;
      vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
    // calculate final mesh matrix
    GPUDrawPushConstants pushConstants;
    pushConstants.worldMatrix = r.transform;
    pushConstants.vertexBuffer = r.vertexBufferAddress;

    vkCmdPushConstants(
      cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &pushConstants);

    //stats
    _stats.drawcallCount++;
    _stats.triangleCount += r.indexCount / 3;
    vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
  };

  _stats.drawcallCount = 0;
  _stats.triangleCount = 0;

  for (auto& r : _mainDrawContext.OpaqueSurfaces)
  {
    draw(r);
  }

  for (auto& r : _mainDrawContext.TransparentSurfaces)
  {
    draw(r);
  }

  // we delete the draw commands now that we processed them
  _mainDrawContext.OpaqueSurfaces.clear();
  _mainDrawContext.TransparentSurfaces.clear();
}

//--------------------------------------------------------------------------------------------------
void VkEngine::drawMain(VkCommandBuffer cmd)
{
  ComputeEffect* effect = _backgroundEffects[_currentBackgroundEffect];

  // bind the background compute pipeline
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect->pipeline);

  // bind the descriptor set containing the draw image for the compute pipeline
  vkCmdBindDescriptorSets(
    cmd,
    VK_PIPELINE_BIND_POINT_COMPUTE,
    _gradientPipelineLayout->_handle,
    0,
    1,
    &_drawImageDescriptors->_handle,
    0,
    nullptr);

  vkCmdPushConstants(
    cmd, _gradientPipelineLayout->_handle, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect->data);
  // execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
  vkCmdDispatch(cmd, std::ceil(_windowExtent.width / 16.0), std::ceil(_windowExtent.height / 16.0), 1);


  VkRenderingAttachmentInfo colorAttachment =
    vkinit::attachmentInfo(_drawImage->_handle.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
  VkRenderingAttachmentInfo depthAttachment =
    vkinit::depthAttachmentInfo(_depthImage->_handle.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  VkRenderingInfo renderInfo = vkinit::renderingInfo(_windowExtent, &colorAttachment, &depthAttachment);

  vkCmdBeginRendering(cmd, &renderInfo);
  auto start = std::chrono::system_clock::now();
  this->drawGeometry(cmd);

  auto end = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  _stats.meshDrawTime = elapsed.count() / 1000.f;

  vkCmdEndRendering(cmd);
}

//--------------------------------------------------------------------------------------------------
GPUMeshBuffers VkEngine::uploadMesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
  const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
  const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

  GPUMeshBuffers newSurface;

  //create vertex buffer
  newSurface.vertexBuffer = this->createBuffer(
    vertexBufferSize,
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    VMA_MEMORY_USAGE_GPU_ONLY);

  //find the address of the vertex buffer
  VkBufferDeviceAddressInfo deviceAdressInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer};
  newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device->getHandle(), &deviceAdressInfo);

  //create index buffer
  newSurface.indexBuffer = this->createBuffer(
    indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

  AllocatedBuffer staging =
    this->createBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

  void* data = staging.allocation->GetMappedData();

  // copy vertex buffer
  memcpy(data, vertices.data(), vertexBufferSize);
  // copy index buffer
  memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

  this->immediateSubmit(
    [&](VkCommandBuffer cmd)
    {
      VkBufferCopy vertexCopy{0};
      vertexCopy.dstOffset = 0;
      vertexCopy.srcOffset = 0;
      vertexCopy.size = vertexBufferSize;

      vkCmdCopyBuffer(cmd, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

      VkBufferCopy indexCopy{0};
      indexCopy.dstOffset = 0;
      indexCopy.srcOffset = vertexBufferSize;
      indexCopy.size = indexBufferSize;

      vkCmdCopyBuffer(cmd, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    });

  this->destroyBuffer(staging);

  return newSurface;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::updateScene()
{
  //begin clock
  auto start = std::chrono::system_clock::now();

  _mainCamera.update();

  auto view = _mainCamera.getViewMatrix();

  _sceneData.view = view;
  _sceneData.lightPosition = _mainLight.position;
  _sceneData.lightColor = _mainLight.color;
  _sceneData.lightPower = _mainLight.power;
  _sceneData.cameraPosition = glm::vec4(_mainCamera.position.x, _mainCamera.position.y, _mainCamera.position.z, 1.0f);
  _sceneData.ambientCoefficient = _mainSurfaceProperties.ambientCoefficient;
  _sceneData.specularCoefficient = _mainSurfaceProperties.specularCoefficient;
  _sceneData.screenGamma = _mainSurfaceProperties.screenGamma;
  _sceneData.shininess = _mainSurfaceProperties.shininess;
  // camera projection
  _sceneData.proj =
    glm::perspective(glm::radians(70.f), (float)_windowExtent.width / (float)_windowExtent.height, 10000.f, 0.1f);

  // invert the Y direction on projection matrix so that we are more similar
  // to opengl and gltf axis
  _sceneData.proj[1][1] *= -1;
  _sceneData.viewproj = _sceneData.proj * _sceneData.view;

  //some default lighting parameters
  _sceneData.ambientColor = glm::vec4(.1f);

  if (!_selectedNodeName.empty())
  {
    _loadedNodes[_selectedNodeName]->Draw(glm::mat4{1.f}, _mainDrawContext);
  }

  if (!_selectedSceneName.empty())
  {
    _loadedScenes[_selectedSceneName]->Draw(glm::mat4{1.f}, _mainDrawContext);
  }

  auto end = std::chrono::system_clock::now();

  //convert to microseconds (integer), and then come back to miliseconds
  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  _stats.sceneUpdateTime = elapsed.count() / 1000.f;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::run()
{
  SDL_Event e;
  bool bQuit = false;

  // main loop
  while (!bQuit)
  {
    //begin clock
    auto start = std::chrono::system_clock::now();

    //Handle events on queue
    while (_window->pollEvent(e) != 0)
    {
      //close the window when user alt-f4s or clicks the X button
      bQuit = _window->processEvent(e);
      _stopRendering = _window->isMinimized();
      _mainCamera.processSDLEvent(e);
      //send SDL event to imgui for handling
      ImGui_ImplSDL2_ProcessEvent(&e);
    }
    if (resize_requested)
    {
      resizeSwapchain();
    }
    //do not draw if we are minimized
    if (_stopRendering)
    {
      //throttle the speed to avoid the endless spinning
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    UserInterface::display(this);

    //our draw function
    this->updateScene();

    this->draw();
    //get clock again, compare with start clock
    auto end = std::chrono::system_clock::now();
    //convert to microseconds (integer), and then come back to miliseconds
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    _stats.frametime = elapsed.count() / 1000.f;
  }
}
//--------------------------------------------------------------------------------------------------
void VkEngine::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
{
  VK_CHECK(vkResetFences(_device->getHandle(), 1, &_immFence->_handle));
  VK_CHECK(vkResetCommandBuffer(_immCommandBuffer->getHandle(), 0));

  VkCommandBuffer cmd = _immCommandBuffer->getHandle();

  VkCommandBufferBeginInfo cmdBeginInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

  function(cmd);

  VK_CHECK(vkEndCommandBuffer(cmd));

  VkCommandBufferSubmitInfo cmdinfo = vkinit::commandBufferSubmitInfo(cmd);
  VkSubmitInfo2 submit = vkinit::submitInfo(&cmdinfo, nullptr, nullptr);

  // submit command buffer to the queue and execute it.
  //  _renderFence will now block until the graphic commands finish execution
  VK_CHECK(vkQueueSubmit2(_device->getGraphicsQueue(), 1, &submit, _immFence->_handle));

  VK_CHECK(vkWaitForFences(_device->getHandle(), 1, &_immFence->_handle, true, 9999999999));
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initVulkan()
{
  _instance = std::make_unique<Instance>(bUseValidationLayers);
  this->createSurface();
  _chosenGPU = std::make_unique<PhysicalDevice>(_instance, _surface);
  _device = std::make_unique<Device>(_chosenGPU);
  this->createMemoryAllocator();
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initSwapchain()
{
  _swapchain = std::make_unique<Swapchain>(_chosenGPU, _device, _surface, _windowExtent.width, _windowExtent.height);
  this->createDrawImage();
  this->createDepthImage();
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initFrameData()
{
  for (int i = 0; i < FRAME_OVERLAP; i++)
  {
    _frames.push_back(std::make_unique<FrameData>(_device));
    _deletionQueue.push([&, i]() { _frames[i]->_frameDescriptors.destroyPools(_device->getHandle()); });
  }
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initImmediateCommands()
{
  _immCommandPool = std::make_unique<CommandPool>(_device);
  _immCommandBuffer = std::make_unique<CommandBuffer>(_device, _immCommandPool);
  _deletionQueue.push([=]() { vkDestroyCommandPool(_device->getHandle(), _immCommandPool->getHandle(), nullptr); });
  _immFence = std::make_unique<Fence>(_device);
  _deletionQueue.push([=]() { vkDestroyFence(_device->getHandle(), _immFence->_handle, nullptr); });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initDescriptors()
{
  //create a descriptor pool that will hold 10 sets with 1 image each
  std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
  };
  _globalDescriptorAllocator.init(_device->getHandle(), 10, sizes);

  //make the descriptor set layout for our compute draw
  _drawImageDescriptorLayout =
    std::make_unique<DescriptorSetLayout>(_device, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

  //make the descriptor set layout for our default texture image
  _singleImageDescriptorLayout =
    std::make_unique<DescriptorSetLayout>(_device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

  _gpuSceneDataDescriptorLayout = std::make_unique<DescriptorSetLayout>(
    _device, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

  //allocate a descriptor set for our draw image
  _drawImageDescriptors = std::make_unique<DescriptorSet>(_device, _drawImageDescriptorLayout, _globalDescriptorAllocator);
  _drawImageDescriptors->writeImage(_device, _drawImage);

  //make sure both the descriptor allocator and the new layout get cleaned up properly
  _deletionQueue.push(
    [&]()
    {
      _drawImageDescriptors->destroyPools(_device);
      vkDestroyDescriptorSetLayout(_device->getHandle(), _drawImageDescriptorLayout->_handle, nullptr);
      vkDestroyDescriptorSetLayout(_device->getHandle(), _singleImageDescriptorLayout->_handle, nullptr);
      vkDestroyDescriptorSetLayout(_device->getHandle(), _gpuSceneDataDescriptorLayout->_handle, nullptr);
    });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initPipelines()
{
  this->initBackgroundPipelines();
  _metalRoughMaterial.buildPipelines(
    _device->getHandle(), _gpuSceneDataDescriptorLayout->_handle, _drawImage->_handle, _depthImage->_handle);
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initBackgroundPipelines()
{
  VkPushConstantRange pushConstant{};
  pushConstant.offset = 0;
  pushConstant.size = sizeof(ComputePushConstants);
  pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  std::vector<VkPushConstantRange> pushConstants;
  pushConstants.push_back(pushConstant);

  _gradientPipelineLayout = std::make_unique<PipelineLayout>(_device, _drawImageDescriptorLayout, pushConstants);

  _gradientPipeline =
    std::make_unique<ComputePipeline>(_device, _gradientPipelineLayout, "../shaders/gradient_color.comp.spv", "gradient");
  _gradientPipeline->_effect.data.data1 = glm::vec4(1, 0, 0, 1);
  _gradientPipeline->_effect.data.data2 = glm::vec4(0, 0, 1, 1);

  _skyPipeline = std::make_unique<ComputePipeline>(_device, _gradientPipelineLayout, "../shaders/sky.comp.spv", "sky");
  _skyPipeline->_effect.data.data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

  //add the 2 background effects into the array
  _backgroundEffects.push_back(&_gradientPipeline->_effect);
  _backgroundEffects.push_back(&_skyPipeline->_effect);

  //destroy structures properly
  vkDestroyShaderModule(_device->getHandle(), _gradientPipeline->_shader, nullptr);
  vkDestroyShaderModule(_device->getHandle(), _skyPipeline->_shader, nullptr);
  _deletionQueue.push(
    [=]()
    {
      vkDestroyPipelineLayout(_device->getHandle(), _gradientPipelineLayout->_handle, nullptr);
      vkDestroyPipeline(_device->getHandle(), _gradientPipeline->_handle, nullptr);
      vkDestroyPipeline(_device->getHandle(), _skyPipeline->_handle, nullptr);
    });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initDefaultData()
{
  //3 default textures, white, grey, black. 1 pixel each
  uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));

  createImage(_whiteImage, (void*)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
  createImage(_greyImage, (void*)&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
  createImage(_blackImage, (void*)&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  //checkerboard image
  uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
  std::array<uint32_t, 16 * 16> pixels;  //for 16x16 checkerboard texture
  for (int x = 0; x < 16; x++)
  {
    for (int y = 0; y < 16; y++)
    {
      pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
    }
  }
  createImage(
    _errorCheckerboardImage, pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

  VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

  sampl.magFilter = VK_FILTER_NEAREST;
  sampl.minFilter = VK_FILTER_NEAREST;

  vkCreateSampler(_device->getHandle(), &sampl, nullptr, &_defaultSamplerNearest);

  sampl.magFilter = VK_FILTER_LINEAR;
  sampl.minFilter = VK_FILTER_LINEAR;
  vkCreateSampler(_device->getHandle(), &sampl, nullptr, &_defaultSamplerLinear);

  _deletionQueue.push(
    [&]()
    {
      vkDestroySampler(_device->getHandle(), _defaultSamplerNearest, nullptr);
      vkDestroySampler(_device->getHandle(), _defaultSamplerLinear, nullptr);

      this->destroyImage(_whiteImage->_handle);
      this->destroyImage(_greyImage->_handle);
      this->destroyImage(_blackImage->_handle);
      this->destroyImage(_errorCheckerboardImage->_handle);
    });

  GLTFMetallicRoughness::MaterialResources materialResources;
  //default the material textures
  materialResources.colorImage = _whiteImage->_handle;
  materialResources.colorSampler = _defaultSamplerLinear;
  materialResources.metalRoughImage = _whiteImage->_handle;
  materialResources.metalRoughSampler = _defaultSamplerLinear;

  //set the uniform buffer for the material data
  AllocatedBuffer materialConstants = createBuffer(
    sizeof(GLTFMetallicRoughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  //write the buffer
  GLTFMetallicRoughness::MaterialConstants* sceneUniformData =
    (GLTFMetallicRoughness::MaterialConstants*)materialConstants.allocation->GetMappedData();
  sceneUniformData->colorFactors = glm::vec4{1, 1, 1, 1};
  sceneUniformData->metalRoughFactors = glm::vec4{1, 0.5, 0, 0};

  _mainSurfaceProperties.ambientCoefficient = 0.1;
  _mainSurfaceProperties.screenGamma = 2.2;
  _mainSurfaceProperties.shininess = 8.0;
  _mainSurfaceProperties.specularCoefficient = 0.5;

  _deletionQueue.push([=, this]() { destroyBuffer(materialConstants); });

  materialResources.dataBuffer = materialConstants.buffer;
  materialResources.dataBufferOffset = 0;

  _defaultData = _metalRoughMaterial.writeMaterial(
    _device->getHandle(), MaterialPass::MainColor, materialResources, _globalDescriptorAllocator);

  _testMeshes = vkloader::loadGltfMeshes(this, "../assets/scaled_teapot.glb").value();

  for (auto& m : _testMeshes)
  {
    std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
    newNode->mesh = m;

    newNode->localTransform = glm::mat4{1.f};
    newNode->worldTransform = glm::mat4{1.f};

    for (auto& s : newNode->mesh->surfaces)
    {
      s.material = std::make_shared<GLTFMaterial>(_defaultData);
    }

    _loadedNodes[m->name] = std::move(newNode);
  }
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initMainCamera()
{
  _mainCamera.velocity = glm::vec3(0.f);
  _mainCamera.position = glm::vec3(0.f, -0.f, 5.f);
  _mainCamera.yaw = 0;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initLight()
{
  _mainLight.position = glm::vec4(0.0f, 10.0f, 0.0f, 0.0f);
  _mainLight.color = glm::vec4(1.0f);
  _mainLight.power = 0.1;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::destroySwapchain()
{
  vkDestroySwapchainKHR(_device->getHandle(), _swapchain->getHandle(), nullptr);

  // destroy swapchain resources
  for (int i = 0; i < _swapchain->getSwapchainImageViews().size(); i++)
  {
    vkDestroyImageView(_device->getHandle(), _swapchain->getSwapchainImageViews()[i], nullptr);
  }
}

//--------------------------------------------------------------------------------------------------
void VkEngine::resizeSwapchain()
{
  vkDeviceWaitIdle(_device->getHandle());

  destroySwapchain();

  int w, h;
  SDL_GetWindowSize(_window->handle(), &w, &h);
  _windowExtent.width = w;
  _windowExtent.height = h;


  _swapchain.reset();
  _swapchain = std::make_unique<Swapchain>(_chosenGPU, _device, _surface, _windowExtent.width, _windowExtent.height);

  resize_requested = false;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createSurface()
{
  if (SDL_Vulkan_CreateSurface(_window->handle(), _instance->getHandle(), &_surface) != SDL_TRUE)
  {
    throw std::runtime_error("failed to create window surface!");
  }
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createMemoryAllocator()
{
  // initialize the memory allocator
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = _chosenGPU->getHandle();
  allocatorInfo.device = _device->getHandle();
  allocatorInfo.instance = _instance->getHandle();
  allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  vmaCreateAllocator(&allocatorInfo, &_allocator);

  _deletionQueue.push([&]() { vmaDestroyAllocator(_allocator); });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createImage(
  std::unique_ptr<Image>& image,
  void* data,
  VkExtent3D size,
  VkFormat format,
  VkImageUsageFlags usage,
  bool mipmapped)
{
  size_t data_size = size.depth * size.width * size.height * 4;
  AllocatedBuffer uploadbuffer = createBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  memcpy(uploadbuffer.info.pMappedData, data, data_size);

  image = std::make_unique<Image>(
    _device, size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, _allocator, mipmapped);

  immediateSubmit(
    [&](VkCommandBuffer cmd)
    {
      vkutil::transitionImage(cmd, image->_handle.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

      VkBufferImageCopy copyRegion = {};
      copyRegion.bufferOffset = 0;
      copyRegion.bufferRowLength = 0;
      copyRegion.bufferImageHeight = 0;

      copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      copyRegion.imageSubresource.mipLevel = 0;
      copyRegion.imageSubresource.baseArrayLayer = 0;
      copyRegion.imageSubresource.layerCount = 1;
      copyRegion.imageExtent = size;

      // copy the buffer into the image
      vkCmdCopyBufferToImage(
        cmd, uploadbuffer.buffer, image->_handle.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

      vkutil::transitionImage(
        cmd, image->_handle.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });

  destroyBuffer(uploadbuffer);
}

//--------------------------------------------------------------------------------------------------
void VkEngine::destroyImage(const AllocatedImage& img)
{
  vkDestroyImageView(_device->getHandle(), img.imageView, nullptr);
  vmaDestroyImage(_allocator, img.image, img.allocation);
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createDrawImage()
{
  //hardcoding the draw format to 32 bit float
  VkFormat imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

  VkImageUsageFlags drawImageUsages{};
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  VkExtent3D imageExtent = {_windowExtent.width, _windowExtent.height, 1};

  _drawImage = std::make_unique<Image>(_device, imageExtent, imageFormat, drawImageUsages, _allocator, false);

  _deletionQueue.push([=]() { vmaDestroyImage(_allocator, _drawImage->_handle.image, _drawImage->_handle.allocation); });
  _deletionQueue.push([=]() { vkDestroyImageView(_device->getHandle(), _drawImage->_handle.imageView, nullptr); });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createDepthImage()
{
  //draw image size will match the window
  VkExtent3D depthImageExtent = {_windowExtent.width, _windowExtent.height, 1};
  VkFormat imageFormat = VK_FORMAT_D32_SFLOAT;
  VkImageUsageFlags depthImageUsages{};
  depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VkExtent3D imageExtent = {_windowExtent.width, _windowExtent.height, 1};

  _depthImage = std::make_unique<Image>(_device, imageExtent, imageFormat, depthImageUsages, _allocator, false);

  _deletionQueue.push([=]() { vmaDestroyImage(_allocator, _depthImage->_handle.image, _depthImage->_handle.allocation); });
  _deletionQueue.push([=]() { vkDestroyImageView(_device->getHandle(), _depthImage->_handle.imageView, nullptr); });
}

//--------------------------------------------------------------------------------------------------
AllocatedBuffer VkEngine::createBuffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
  // allocate buffer
  VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.pNext = nullptr;
  bufferInfo.size = allocSize;

  bufferInfo.usage = usage;

  VmaAllocationCreateInfo vmaallocInfo = {};
  vmaallocInfo.usage = memoryUsage;
  vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
  AllocatedBuffer newBuffer;

  // allocate the buffer
  VK_CHECK(
    vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

  return newBuffer;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::destroyBuffer(const AllocatedBuffer& buffer)
{
  vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

//--------------------------------------------------------------------------------------------------
void MeshNode::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
  glm::mat4 nodeMatrix = topMatrix * worldTransform;

  for (auto& s : mesh->surfaces)
  {
    RenderObject def;
    def.indexCount = s.count;
    def.firstIndex = s.startIndex;
    def.indexBuffer = mesh->meshBuffers.indexBuffer.buffer;
    def.material = &s.material->data;
    def.bounds = s.bounds;
    def.transform = nodeMatrix;
    def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

    if (s.material->data.passType == MaterialPass::Transparent)
    {
      ctx.TransparentSurfaces.push_back(def);
    }
    else
    {
      ctx.OpaqueSurfaces.push_back(def);
    }
  }

  // recurse down
  Node::Draw(topMatrix, ctx);
}
