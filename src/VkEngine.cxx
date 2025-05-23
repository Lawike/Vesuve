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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/packing.hpp>
#include <set>
#include <thread>
#include "DebugUtils.hpp"
#include "PipelineLayout.hpp"
#include "SingleTimeCommand.hpp"
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

template<class TAccelerationStructure> VkAccelerationStructureBuildSizesInfoKHR GetTotalRequirements(
  const std::vector<TAccelerationStructure>& accelerationStructures)
{
  VkAccelerationStructureBuildSizesInfoKHR total{};

  for (const auto& accelerationStructure : accelerationStructures)
  {
    total.accelerationStructureSize += accelerationStructure._buildSizesInfo.accelerationStructureSize;
    total.buildScratchSize += accelerationStructure._buildSizesInfo.buildScratchSize;
    total.updateScratchSize += accelerationStructure._buildSizesInfo.updateScratchSize;
  }

  return total;
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
  this->initRaytracingDescriptors();
  this->initRaytracingPipeline();
  this->initShaderBindingTable();
  this->initAccelerationStructures();
  this->updateRaytracingDescriptors();
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
      vkDestroyFence(_device->getHandle(), _frames[i]->_presentFence->_handle, nullptr);
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
  if (_isRaytracingEnabled != _isPreviousFrameRT)
  {
    this->resetFrame();
  }
  this->updateFrame();

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
    _resize_requested = true;
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
  std::string swapSemaphoreIndex = "swapchain semaphore index :" + std::to_string(swapchainImageIndex);
  DebugUtils::SetObjectName(
    this->getCurrentFrame()->_swapchainSemaphore->_handle, swapSemaphoreIndex.c_str(), _device->getHandle());


  VkSemaphoreSubmitInfo signalInfo =
    vkinit::semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, this->getCurrentFrame()->_renderSemaphore->_handle);

  VkSubmitInfo2 submit = vkinit::submitInfo(&cmdinfo, &signalInfo, &waitInfo);

  //submit command buffer to the queue and execute it.
  // _renderFence will now block until the graphic commands finish execution

  VK_CHECK(vkQueueSubmit2(_device->getGraphicsQueue(), 1, &submit, this->getCurrentFrame()->_renderFence->_handle));

  VK_CHECK(vkResetFences(_device->getHandle(), 1, &(this->getCurrentFrame()->_presentFence->_handle)));

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

  //DEBUG
  std::string rendSemaphoreIndex = "Render semaphore index :" + std::to_string(swapchainImageIndex);
  DebugUtils::SetObjectName(
    this->getCurrentFrame()->_renderSemaphore->_handle, rendSemaphoreIndex.c_str(), _device->getHandle());

  VkSwapchainPresentFenceInfoEXT presentFenceInfo = {};
  presentFenceInfo.pFences = &(this->getCurrentFrame()->_presentFence->_handle);
  presentFenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
  presentFenceInfo.swapchainCount = 1;
  presentInfo.pNext = &presentFenceInfo;

  VkResult presentResult = vkQueuePresentKHR(_device->getGraphicsQueue(), &presentInfo);
  if (e == VK_ERROR_OUT_OF_DATE_KHR)
  {
    _resize_requested = true;
    return;
  }
  VK_CHECK(vkWaitForFences(_device->getHandle(), 1, &(this->getCurrentFrame()->_presentFence->_handle), true, 9999999999));
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
void VkEngine::drawRaytracing(VkCommandBuffer cmd)
{
  std::vector<VkDescriptorSet> descriptorSets{_raytracingDescriptorSet->_handle, _gpuSceneDataDescriptorSet->_handle};

  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.baseArrayLayer = 0;
  subresourceRange.layerCount = 1;

  // Acquire destination images for rendering.
  VkImageMemoryBarrier accumulationBarrier;
  accumulationBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  accumulationBarrier.pNext = nullptr;
  accumulationBarrier.srcAccessMask = 0;
  accumulationBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  accumulationBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  accumulationBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  accumulationBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  accumulationBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  accumulationBarrier.image = _accumulationImage->_handle.image;
  accumulationBarrier.subresourceRange = subresourceRange;

  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    0,
    0,
    nullptr,
    0,
    nullptr,
    1,
    &accumulationBarrier);

  // Acquire destination images for rendering.
  VkImageMemoryBarrier drawBarrier;
  drawBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  drawBarrier.pNext = nullptr;
  drawBarrier.srcAccessMask = 0;
  drawBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  drawBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  drawBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  drawBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  drawBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  drawBarrier.image = _drawImage->_handle.image;
  drawBarrier.subresourceRange = subresourceRange;

  vkCmdPipelineBarrier(
    cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &drawBarrier);


  // Bind ray tracing pipeline.
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _raytracingPipeline->_handle);
  vkCmdBindDescriptorSets(
    cmd,
    VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
    _raytracingPipelineLayout->_handle,
    0,
    descriptorSets.size(),
    descriptorSets.data(),
    0,
    nullptr);

  RaytracingPushConstant rtPushConstant{};
  rtPushConstant.vertexBufferAddress = _testMeshes[_selectedMeshIndex]->meshBuffers.vertexBufferAddress;
  rtPushConstant.indexBufferAddress = _testMeshes[_selectedMeshIndex]->meshBuffers.indexBufferAddress;

  vkCmdPushConstants(
    cmd,
    _raytracingPipelineLayout->_handle,
    VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    0,
    sizeof(RaytracingPushConstant),
    &rtPushConstant);

  // Describe the shader binding table.
  VkStridedDeviceAddressRegionKHR raygenShaderBindingTable = {};
  raygenShaderBindingTable.deviceAddress = _shaderBindingTable->_raygenShaderAddress;
  raygenShaderBindingTable.stride = _shaderBindingTable->_raygenEntrySize;
  raygenShaderBindingTable.size = _shaderBindingTable->_raygenSize;

  VkStridedDeviceAddressRegionKHR missShaderBindingTable = {};
  missShaderBindingTable.deviceAddress = _shaderBindingTable->_missShaderAddress;
  missShaderBindingTable.stride = _shaderBindingTable->_missEntrySize;
  missShaderBindingTable.size = _shaderBindingTable->_missSize;

  VkStridedDeviceAddressRegionKHR hitShaderBindingTable = {};
  hitShaderBindingTable.deviceAddress = _shaderBindingTable->_closesHitShaderAddress;
  hitShaderBindingTable.stride = _shaderBindingTable->_hitGroupEntrySize;
  hitShaderBindingTable.size = _shaderBindingTable->_hitGroupSize;

  VkStridedDeviceAddressRegionKHR callableShaderBindingTable = {};

  // Execute ray tracing shaders.
  auto cmdTraceRaysKHR = vkloader::loadFunction<PFN_vkCmdTraceRaysKHR>(_device->getHandle(), "vkCmdTraceRaysKHR");
  cmdTraceRaysKHR(
    cmd,
    &raygenShaderBindingTable,
    &missShaderBindingTable,
    &hitShaderBindingTable,
    &callableShaderBindingTable,
    _windowExtent.width,
    _windowExtent.height,
    1);
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
          cmd,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          r.material->pipeline->layout,
          0,
          1,
          &_gpuSceneDataDescriptorSet->_handle,
          0,
          nullptr);

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
  VkRenderingAttachmentInfo colorAttachment =
    vkinit::attachmentInfo(_drawImage->_handle.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
  VkRenderingAttachmentInfo depthAttachment =
    vkinit::depthAttachmentInfo(_depthImage->_handle.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

  _gpuSceneDataDescriptorSet =
    std::make_unique<DescriptorSet>(_device, _gpuSceneDataDescriptorLayout, _globalDescriptorAllocator);

  //allocate a new uniform buffer for the scene data
  AllocatedBuffer gpuSceneDataBuffer =
    this->createBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

  //add it to the deletion queue of this frame so it gets deleted once its been used
  this->getCurrentFrame()->_deletionQueue.push([=, this]() { this->destroyBuffer(gpuSceneDataBuffer); });

  //get the memory mapped data using vma
  GPUSceneData* sceneUniformData = (GPUSceneData*)gpuSceneDataBuffer.allocation->GetMappedData();

  //write the data into the buffer
  _gpuSceneDataDescriptorSet->writeUniformBuffer(
    _device, gpuSceneDataBuffer, sceneUniformData, _sceneData, 0, sizeof(GPUSceneData), 0);

  VkRenderingInfo renderInfo = vkinit::renderingInfo(_windowExtent, &colorAttachment, &depthAttachment);
  // Draw either blinn phong or ray tracing.
  if (!_isRaytracingEnabled)
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

    vkCmdBeginRendering(cmd, &renderInfo);
    auto start = std::chrono::system_clock::now();

    this->drawGeometry(cmd);
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    _stats.meshDrawTime = elapsed.count() / 1000.f;

    vkCmdEndRendering(cmd);
    _isPreviousFrameRT = false;
  }
  else
  {
    if (_frameNumber < maxNbOfFramesRT)
    {
      this->drawRaytracing(cmd);
    }
    _isPreviousFrameRT = true;
  }
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
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VMA_MEMORY_USAGE_AUTO);
  newSurface.vertexCount = vertices.size();

  //find the address of the vertex buffer
  VkBufferDeviceAddressInfo vertexAdressInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer};
  newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(_device->getHandle(), &vertexAdressInfo);

  //create index buffer
  newSurface.indexBuffer = this->createBuffer(
    indexBufferSize,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VMA_MEMORY_USAGE_AUTO);
  newSurface.indexCount = indices.size();

  //find the address of the index buffer
  VkBufferDeviceAddressInfo indexAdressInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.indexBuffer.buffer};
  newSurface.indexBufferAddress = vkGetBufferDeviceAddress(_device->getHandle(), &indexAdressInfo);
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
  _sceneData.invView = glm::inverse(view);
  _sceneData.lightPosition = _mainLight.position;
  _sceneData.lightColor = _mainLight.color;
  _sceneData.lightPower = _mainLight.power;
  _sceneData.cameraPosition = glm::vec4(_mainCamera.position.x, _mainCamera.position.y, _mainCamera.position.z, 1.0f);
  _sceneData.ambientCoefficient = _mainSurfaceProperties.ambientCoefficient;
  _sceneData.specularCoefficient = _mainSurfaceProperties.specularCoefficient;
  _sceneData.screenGamma = _mainSurfaceProperties.screenGamma;
  _sceneData.shininess = _mainSurfaceProperties.shininess;
  // camera projection
  float fov = 70.f;
  float aspect = (float)_windowExtent.width / (float)_windowExtent.height;
  float near = 0.1f;
  float far = 1000.0f;
  _sceneData.proj = glm::perspective(fov, aspect, near, far);
  _sceneData.proj[1][1] *= -1;
  _sceneData.invProj = glm::inverse(_sceneData.proj);

  // invert the Y direction on projection matrix so that we are more similar
  // to opengl and gltf axis
  _sceneData.viewproj = _sceneData.proj * _sceneData.view;

  //some default lighting parameters
  _sceneData.ambientColor = glm::vec4(.1f);
  _sceneData.frameIndex = _frameNumber;


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
    if (_resize_requested)
    {
      resetFrame();
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
void VkEngine::immediateSubmit(std::function<void(VkCommandBuffer)>&& function)
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
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}};

  _globalDescriptorAllocator.init(_device->getHandle(), 10, sizes);

  //make the descriptor set layout for our compute draw
  std::vector<DescriptorBinding> drawImageBindings = {{0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}};
  _drawImageDescriptorLayout = std::make_unique<DescriptorSetLayout>(_device, drawImageBindings);

  //make the descriptor set layout for our default texture image
  std::vector<DescriptorBinding> singleImageBindings = {
    {0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}};
  _singleImageDescriptorLayout = std::make_unique<DescriptorSetLayout>(_device, singleImageBindings);

  std::vector<DescriptorBinding> sceneDataBindings = {
    // Camera matrices
    {0,
     1,
     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
       VK_SHADER_STAGE_FRAGMENT_BIT}};
  _gpuSceneDataDescriptorLayout = std::make_unique<DescriptorSetLayout>(_device, sceneDataBindings);

  //allocate a descriptor set for our draw image
  _drawImageDescriptors = std::make_unique<DescriptorSet>(_device, _drawImageDescriptorLayout, _globalDescriptorAllocator);
  _drawImageDescriptors->writeImage(_device, _drawImage);
  _drawImageDescriptors->updateSet(_device);
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
void VkEngine::initRaytracingDescriptors()
{
  std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
    // Top level acceleration structure.
    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
    // Image accumulation & output
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
  };
  std::vector<DescriptorBinding> bindings{
    // Top level acceleration structure.
    {0,
     1,
     VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
     VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
    // Output image
    {1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR},
  };
  _raytracingDescriptorAllocator.init(_device->getHandle(), 1, sizes);
  _raytracingDescriptorSetLayout = std::make_unique<DescriptorSetLayout>(_device, bindings);
  _raytracingDescriptorSet =
    std::make_unique<DescriptorSet>(_device, _raytracingDescriptorSetLayout, _raytracingDescriptorAllocator);

  //make sure both the descriptor allocator and the new layout get cleaned up properly
  _deletionQueue.push(
    [&]()
    {
      _raytracingDescriptorSet->destroyPools(_device);
      vkDestroyDescriptorSetLayout(_device->getHandle(), _raytracingDescriptorSetLayout->_handle, nullptr);
    });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::updateRaytracingDescriptors()
{
  // Write acceleration structure
  VkWriteDescriptorSetAccelerationStructureKHR descASInfo{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
  descASInfo.accelerationStructureCount = 1;
  descASInfo.pAccelerationStructures = &_topAS[0]._handle;
  _raytracingDescriptorSet->writeAccelerationStructure(_device, _topAS[0], 0, descASInfo);

  // Write output image
  _raytracingDescriptorSet->writeImage(_device, _drawImage, 1);
  _raytracingDescriptorSet->updateSet(_device);
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

  std::vector<VkDescriptorSetLayout> descriptors = {_drawImageDescriptorLayout->_handle};

  _gradientPipelineLayout = std::make_unique<PipelineLayout>(_device, descriptors, pushConstants);

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
void VkEngine::initRaytracingPipeline()
{
  VkFormat imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
  VkImageUsageFlags drawImageUsages{};
  drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
  VkExtent3D imageExtent{_windowExtent.width, _windowExtent.height, 1};

  _accumulationImage = std::make_unique<Image>(_device, imageExtent, imageFormat, drawImageUsages, _allocator, false);
  std::vector<VkDescriptorSetLayout> descriptors = {
    _raytracingDescriptorSetLayout->_handle, _gpuSceneDataDescriptorLayout->_handle};

  std::vector<VkPushConstantRange> pushConstants;
  VkPushConstantRange pc;
  pc.offset = 0;
  pc.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  pc.size = sizeof(RaytracingPushConstant);
  pushConstants.emplace_back(pc);
  _raytracingPipelineLayout = std::make_unique<PipelineLayout>(_device, descriptors, pushConstants);

  // Load shaders
  std::string raygenShader = "../shaders/raygen.rgen.spv";
  std::string missShader = "../shaders/miss.rmiss.spv";
  std::string shadowMissShader = "../shaders/shadow.rmiss.spv";
  std::string closestHitShader = "../shaders/closestHit.rchit.spv";
  std::string proceduralClosestHitShader = "../shaders/proceduralClosestHit.rchit.spv";
  std::string proceduralIntersectionShader = "../shaders/proceduralIntersection.rint.spv";

  _raytracingPipeline = std::make_unique<RaytracingPipeline>(
    _device,
    _raytracingPipelineLayout,
    raygenShader,
    missShader,
    shadowMissShader,
    closestHitShader,
    proceduralClosestHitShader,
    proceduralIntersectionShader);

  _deletionQueue.push(
    [=]()
    {
      vmaDestroyImage(_allocator, _accumulationImage->_handle.image, _accumulationImage->_handle.allocation);
      vkDestroyImageView(_device->getHandle(), _accumulationImage->_handle.imageView, nullptr);
      vkDestroyPipelineLayout(_device->getHandle(), _raytracingPipelineLayout->_handle, nullptr);
      vkDestroyPipeline(_device->getHandle(), _raytracingPipeline->_handle, nullptr);
    });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initShaderBindingTable()
{
  _raytracingProperties = std::make_unique<RaytracingProperties>(_chosenGPU);
  const std::vector<ShaderBindingTable::Entry> rayGenPrograms = {{_raytracingPipeline->_raygenGroupIndex, {}}};
  const std::vector<ShaderBindingTable::Entry> missPrograms = {
    {_raytracingPipeline->_missGroupIndex, {}}, {{_raytracingPipeline->_shadowMissGroupIndex}, {}}};
  const std::vector<ShaderBindingTable::Entry> hitGroups = {
    {_raytracingPipeline->_triangleHitGroupIndex, {}}, {_raytracingPipeline->_proceduralHitGroupIndex, {}}};
  _shaderBindingTable = std::make_unique<ShaderBindingTable>(
    _device, _allocator, _raytracingProperties, _raytracingPipeline, rayGenPrograms, missPrograms, hitGroups);
  _deletionQueue.push([=]() { this->destroyBuffer(_shaderBindingTable->_handle); });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::initAccelerationStructures()
{
  std::unique_ptr<CommandPool> pool = std::make_unique<CommandPool>(_device);
  std::unique_ptr<SingleTimeCommand> cmdBottom =
    std::make_unique<SingleTimeCommand>(_device->getHandle(), pool->getHandle(), _device->getGraphicsQueue());

  cmdBottom->begin();
  DebugUtils::SetObjectName(
    cmdBottom->buffer, "Single Time Bottom Acceleration structure command buffer", _device->getHandle().device);
  createBottomLevelStructures(cmdBottom->buffer);
  cmdBottom->end();


  std::unique_ptr<SingleTimeCommand> cmdTop =
    std::make_unique<SingleTimeCommand>(_device->getHandle(), pool->getHandle(), _device->getGraphicsQueue());
  cmdTop->begin();
  DebugUtils::SetObjectName(
    cmdTop->buffer, "Single Time Top Acceleration structure command buffer", _device->getHandle().device);
  createTopLevelStructures(cmdTop->buffer);
  cmdTop->end();
  vkDestroyCommandPool(_device->getHandle(), pool->getHandle(), nullptr);
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createBottomLevelStructures(VkCommandBuffer cmd)
{
  // Bottom level acceleration structure
  // Triangles via vertex buffers.
  uint32_t vertexOffset = 0;
  uint32_t indexOffset = 0;
  for (auto mesh : _testMeshes)
  {
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> offsetInfos;
    std::vector<VkAccelerationStructureGeometryKHR> geometries;
    // Only triangle meshes for now
    VkAccelerationStructureGeometryTrianglesDataKHR triangles{
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
    triangles.pNext = nullptr;
    triangles.vertexData.deviceAddress = mesh->meshBuffers.vertexBufferAddress;
    triangles.vertexStride = sizeof(Vertex);
    triangles.maxVertex = mesh->meshBuffers.vertexCount;
    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    triangles.indexData.deviceAddress = mesh->meshBuffers.indexBufferAddress;
    triangles.indexType = VK_INDEX_TYPE_UINT32;
    // Indicate identity transform by setting transformData to null device pointer.
    triangles.transformData = {};

    // General geometry container described as containing opaque triangles
    VkAccelerationStructureGeometryKHR geometry = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometry.triangles = triangles;
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    geometries.push_back(geometry);

    VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
    buildOffsetInfo.firstVertex = vertexOffset / sizeof(Vertex);
    buildOffsetInfo.primitiveOffset = indexOffset;
    buildOffsetInfo.primitiveCount = mesh->meshBuffers.indexCount / 3;  // Triangles => 3 vertices
    buildOffsetInfo.transformOffset = 0;

    offsetInfos.push_back(buildOffsetInfo);
    _bottomAS.emplace_back(BottomLevelAccelerationStructure{_device, _raytracingProperties, geometries, offsetInfos});

    vertexOffset += mesh->meshBuffers.vertexCount * sizeof(Vertex);
    indexOffset += mesh->meshBuffers.indexCount * sizeof(uint32_t);
  }

  // Allocate memory for bottom acceleration structure
  const auto total = GetTotalRequirements(_bottomAS);
  _bottomBuffer = this->createBuffer(
    total.accelerationStructureSize,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    VMA_MEMORY_USAGE_AUTO);
  DebugUtils::SetObjectName(_bottomBuffer.buffer, "BLAS structure buffer", _device->getHandle());

  _scratchBuffer = this->createBuffer(
    total.buildScratchSize,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VMA_MEMORY_USAGE_AUTO);
  DebugUtils::SetObjectName(_bottomBuffer.buffer, "BLAS scratch buffer", _device->getHandle());

  // Generate the structures.
  VkDeviceSize resultOffset = 0;
  VkDeviceSize scratchOffset = 0;

  int index = 0;
  for (BottomLevelAccelerationStructure& accelerationStructure : _bottomAS)
  {
    accelerationStructure.Generate(_device, cmd, _scratchBuffer, scratchOffset, _bottomBuffer, resultOffset);
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(
      cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    DebugUtils::SetObjectName(
      accelerationStructure._handle, ("BLAS #" + std::to_string(index)).c_str(), _device->getHandle());

    resultOffset += accelerationStructure._buildSizesInfo.accelerationStructureSize;
    scratchOffset += accelerationStructure._buildSizesInfo.buildScratchSize;
  }

  // Fill deletion queue with Acceleration structure
  _deletionQueue.push(
    [=]()
    {
      for (auto accelerationStructure : _bottomAS)
      {
        auto destroyAccelerationStructureKHR = vkloader::loadFunction<PFN_vkDestroyAccelerationStructureKHR>(
          _device->getHandle(), "vkDestroyAccelerationStructureKHR");
        destroyAccelerationStructureKHR(_device->getHandle(), accelerationStructure._handle, nullptr);
      }
      vmaDestroyBuffer(_allocator, _bottomBuffer.buffer, _bottomBuffer.allocation);
      vmaDestroyBuffer(_allocator, _scratchBuffer.buffer, _scratchBuffer.allocation);
    });
}

//--------------------------------------------------------------------------------------------------
void VkEngine::createTopLevelStructures(VkCommandBuffer cmd)
{
  //Top level acceleration structure
  std::vector<VkAccelerationStructureInstanceKHR> instances;

  // Hit group 0: triangles
  // Hit group 1: procedurals for now not implemented
  uint32_t instanceId = 0;

  for (auto mesh : _testMeshes)
  {
    instances.push_back(
      TopLevelAccelerationStructure::CreateInstance(_device, _bottomAS[instanceId], glm::mat4(1), instanceId, 0));
    instanceId++;
  }
  const VkMemoryAllocateFlags allocateFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                              VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
  const auto contentSize = sizeof(instances[0]) * instances.size();

  _instancesBuffer = this->createBuffer(contentSize, allocateFlags, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  // Create and copy instances buffer (do it in a separate one-time synchronous command buffer).
  this->copyBuffer(cmd, _instancesBuffer, instances);

  // Make sure the copy of the instance buffer are copied before triggering the acceleration structure build
  VkMemoryBarrier copyBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  copyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  copyBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    0,
    1,
    &copyBarrier,
    0,
    nullptr,
    0,
    nullptr);
  VkBufferDeviceAddressInfo addressInfo = {};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.pNext = nullptr;
  addressInfo.buffer = _instancesBuffer.buffer;
  VkDeviceAddress instancesBufferAdress = vkGetBufferDeviceAddress(_device->getHandle(), &addressInfo);
  _topAS.emplace_back(
    TopLevelAccelerationStructure{
      _device, _raytracingProperties, _instancesBuffer, 0, instancesBufferAdress, static_cast<uint32_t>(instances.size())});

  // Allocate the structure memory.
  const auto total = GetTotalRequirements(_topAS);
  _topBuffer = this->createBuffer(
    total.accelerationStructureSize,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  _topScratchBuffer = this->createBuffer(
    total.buildScratchSize,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

  // Generate the structures.
  _topAS[0].Generate(_device, cmd, _topScratchBuffer, 0, _topBuffer, 0);

  // Make sure to have the TLAS ready before using it
  VkMemoryBarrier readyBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
  readyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  readyBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    0,
    1,
    &readyBarrier,
    0,
    nullptr,
    0,
    nullptr);

  _deletionQueue.push(
    [=]()
    {
      for (auto accelerationStructure : _topAS)
      {
        auto destroyAccelerationStructureKHR = vkloader::loadFunction<PFN_vkDestroyAccelerationStructureKHR>(
          _device->getHandle(), "vkDestroyAccelerationStructureKHR");
        destroyAccelerationStructureKHR(_device->getHandle(), accelerationStructure._handle, nullptr);
      }
      vmaDestroyBuffer(_allocator, _instancesBuffer.buffer, _instancesBuffer.allocation);
      vmaDestroyBuffer(_allocator, _topBuffer.buffer, _topBuffer.allocation);
      vmaDestroyBuffer(_allocator, _topScratchBuffer.buffer, _topScratchBuffer.allocation);
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
  _mainSurfaceProperties.specularCoefficient = 3.14;

  _deletionQueue.push([=, this]() { destroyBuffer(materialConstants); });

  materialResources.dataBuffer = materialConstants.buffer;
  materialResources.dataBufferOffset = 0;

  _defaultData = _metalRoughMaterial.writeMaterial(
    _device->getHandle(), MaterialPass::MainColor, materialResources, _globalDescriptorAllocator);

  //MeshAsset triangleMesh = createTestTriangleMesh();
  //MeshAsset quadMesh = createTestQuadMesh();

  _testMeshes = vkloader::loadGltfMeshes(this, "../assets/scaled_teapot.glb").value();
  //_testMeshes.emplace_back(std::make_shared<MeshAsset>(std::move(triangleMesh)));
  //_testMeshes.emplace_back(std::make_shared<MeshAsset>(std::move(quadMesh)));
  //_testMeshes = vkloader::loadGltfMeshes(this, "../assets/cube.glb").value();
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
  _mainLight.position = glm::vec4(5.0f, 5.0f, 5.0f, 0.0f);
  _mainLight.color = glm::vec4(1.0f);
  _mainLight.power = 1;
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

  _resize_requested = false;
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
  VkFormat imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

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
void VkEngine::resetFrame()
{
  _frameNumber = -1;
}

//--------------------------------------------------------------------------------------------------
void VkEngine::updateFrame()
{
  static glm::mat4 refCamMatrix;

  const auto& m = _mainCamera.getViewMatrix();

  if (refCamMatrix != m)
  {
    resetFrame();
    refCamMatrix = m;
  }
  _frameNumber++;
}

//--------------------------------------------------------------------------------------------------
MeshAsset VkEngine::createTestTriangleMesh()
{
  // Debug item
  MeshAsset testTriangle;
  // Vertices & indices
  Vertex v0;
  Vertex v1;
  Vertex v2;
  std::vector<Vertex> vertices;
  v0.position = glm::vec3(0.f, 1.f, 0.f);
  v0.normal = glm::vec3(0.f, 0.f, 1.f);
  v0.uv_x = 0.5f;
  v0.uv_y = 1.f;
  v0.color = glm::vec4(1.f, 0.f, 0.f, 0.f);

  v1.position = glm::vec3(1.f, 0.f, 0.f);
  v1.normal = glm::vec3(0.f, 0.f, 1.f);
  v1.uv_x = 1.f;
  v1.uv_y = 0.f;
  v1.color = glm::vec4(0.f, 1.f, 0.f, 0.f);

  v2.position = glm::vec3(-1.f, 0.f, 0.f);
  v2.normal = glm::vec3(0.f, 0.f, 1.f);
  v2.uv_x = 0.f;
  v2.uv_y = 0.f;
  v2.color = glm::vec4(0.f, 1.f, 0.f, 0.f);

  vertices = {v0, v1, v2};
  std::vector<uint32_t> indices = {0, 1, 2};

  testTriangle.meshBuffers = uploadMesh(indices, vertices);
  testTriangle.name = "Triangle";

  GeoSurface newSurface;
  newSurface.startIndex = 0;
  newSurface.count = 3;

  testTriangle.surfaces.push_back(newSurface);

  return testTriangle;
}

//--------------------------------------------------------------------------------------------------
MeshAsset VkEngine::createTestQuadMesh()
{
  // Debug item
  MeshAsset testQuad;
  // Vertices & indices
  Vertex v0;
  Vertex v1;
  Vertex v2;
  Vertex v3;
  std::vector<Vertex> vertices;
  v0.position = glm::vec3(-1.f, 1.f, 0.f);
  v0.normal = glm::vec3(0.f, 0.f, 1.f);
  v0.uv_x = 0.f;
  v0.uv_y = 0.f;
  v0.color = glm::vec4(1.f, 0.f, 0.f, 0.f);

  v1.position = glm::vec3(1.f, 1.f, 0.f);
  v1.normal = glm::vec3(0.f, 0.f, 1.f);
  v1.uv_x = 1.f;
  v1.uv_y = 0.f;
  v1.color = glm::vec4(0.f, 1.f, 0.f, 0.f);

  v2.position = glm::vec3(-1.f, -1.f, 0.f);
  v2.normal = glm::vec3(0.f, 0.f, 1.f);
  v2.uv_x = 0.f;
  v2.uv_y = 0.f;
  v2.color = glm::vec4(0.f, 1.f, 0.f, 0.f);

  v3.position = glm::vec3(1.f, -1.f, 0.f);
  v3.normal = glm::vec3(0.f, 0.f, 1.f);
  v3.uv_x = 0.f;
  v3.uv_y = 0.f;
  v3.color = glm::vec4(1.f, 1.f, 1.f, 0.f);

  vertices = {v0, v1, v2, v3};
  std::vector<uint32_t> indices = {0, 1, 2, 3, 2, 1};

  testQuad.meshBuffers = uploadMesh(indices, vertices);
  testQuad.name = "Quad";

  GeoSurface newSurface;
  newSurface.startIndex = 0;
  newSurface.count = 6;

  testQuad.surfaces.push_back(newSurface);

  return testQuad;
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
  vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  AllocatedBuffer newBuffer;

  // allocate the buffer
  VK_CHECK(
    vmaCreateBuffer(_allocator, &bufferInfo, &vmaallocInfo, &newBuffer.buffer, &newBuffer.allocation, &newBuffer.info));

  return newBuffer;
}

//--------------------------------------------------------------------------------------------------
template<class T> void VkEngine::copyBuffer(VkCommandBuffer cmd, AllocatedBuffer& dstBuffer, const std::vector<T>& content)
{
  const auto contentSize = sizeof(content[0]) * content.size();
  // Create a temporary host-visible staging buffer.
  auto stagingBuffer = this->createBuffer(contentSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST);

  // Copy the host data into the staging buffer.
  void* data;
  VK_CHECK(vmaMapMemory(_allocator, stagingBuffer.allocation, &data));
  std::memcpy(data, content.data(), contentSize);
  vmaUnmapMemory(_allocator, stagingBuffer.allocation);


  // Copy the staging buffer to the device buffer.
  this->immediateSubmit(
    [&](VkCommandBuffer cmd)
    {
      VkBufferCopy bufferCopy{};
      bufferCopy.dstOffset = 0;
      bufferCopy.srcOffset = 0;
      bufferCopy.size = contentSize;

      vkCmdCopyBuffer(cmd, stagingBuffer.buffer, dstBuffer.buffer, 1, &bufferCopy);
    });
  this->destroyBuffer(stagingBuffer);
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
