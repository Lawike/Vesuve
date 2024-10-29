#include "Materials.hpp"
#include "VkInitializers.hpp"
#include "VkPipelines.hpp"

//--------------------------------------------------------------------------------------------------
void GLTFMetallicRoughness::buildPipelines(
  VkDevice device,
  VkDescriptorSetLayout gpuSceneDataDescriptorLayout,
  AllocatedImage drawImage,
  AllocatedImage depthImage)
{
  VkShaderModule meshFragShader;
  if (!vkutil::loadShaderModule("../shaders/blinn_phong.frag.spv", device, &meshFragShader))
  {
    fmt::println("Error when building the triangle fragment shader module");
  }

  VkShaderModule meshVertexShader;
  if (!vkutil::loadShaderModule("../shaders/mesh.vert.spv", device, &meshVertexShader))
  {
    fmt::println("Error when building the triangle vertex shader module");
  }

  VkPushConstantRange matrixRange{};
  matrixRange.offset = 0;
  matrixRange.size = sizeof(GPUDrawPushConstants);
  matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  DescriptorLayoutBuilder layoutBuilder;
  layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  _materialLayout = layoutBuilder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

  VkDescriptorSetLayout layouts[] = {gpuSceneDataDescriptorLayout, _materialLayout};

  VkPipelineLayoutCreateInfo meshLLayoutInfo = vkinit::pipelineLayoutCreateInfo();
  meshLLayoutInfo.setLayoutCount = 2;
  meshLLayoutInfo.pSetLayouts = layouts;
  meshLLayoutInfo.pPushConstantRanges = &matrixRange;
  meshLLayoutInfo.pushConstantRangeCount = 1;

  VkPipelineLayout newLayout;
  VK_CHECK(vkCreatePipelineLayout(device, &meshLLayoutInfo, nullptr, &newLayout));

  _opaquePipeline.layout = newLayout;
  _transparentPipeline.layout = newLayout;

  // build the stage-create-info for both vertex and fragment stages. This lets
  // the pipeline know the shader modules per stage
  PipelineBuilder pipelineBuilder;
  pipelineBuilder.setShaders(meshVertexShader, meshFragShader);
  pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
  pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
  pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
  pipelineBuilder.setMultisamplingNone();
  pipelineBuilder.disableBlending();
  pipelineBuilder.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

  //render format
  pipelineBuilder.setColorAttachmentFormat(drawImage.imageFormat);
  pipelineBuilder.setDepthFormat(depthImage.imageFormat);

  // use the triangle layout we created
  pipelineBuilder._pipelineLayout = newLayout;

  // finally build the pipeline
  _opaquePipeline.pipeline = pipelineBuilder.buildPipeline(device);

  // create the transparent variant
  pipelineBuilder.enableBlendingAdditive();

  pipelineBuilder.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

  _transparentPipeline.pipeline = pipelineBuilder.buildPipeline(device);

  vkDestroyShaderModule(device, meshFragShader, nullptr);
  vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

//--------------------------------------------------------------------------------------------------
void GLTFMetallicRoughness::clearResources(VkDevice device)
{
  {
    vkDestroyDescriptorSetLayout(device, _materialLayout, nullptr);
    vkDestroyPipelineLayout(device, _transparentPipeline.layout, nullptr);

    vkDestroyPipeline(device, _transparentPipeline.pipeline, nullptr);
    vkDestroyPipeline(device, _opaquePipeline.pipeline, nullptr);
  }
}

//--------------------------------------------------------------------------------------------------
MaterialInstance GLTFMetallicRoughness::writeMaterial(
  VkDevice device,
  MaterialPass pass,
  const MaterialResources& resources,
  DescriptorAllocatorGrowable& descriptorAllocator)
{
  MaterialInstance matData;
  matData.passType = pass;
  if (pass == MaterialPass::Transparent)
  {
    matData.pipeline = &_transparentPipeline;
  }
  else
  {
    matData.pipeline = &_opaquePipeline;
  }

  matData.materialSet = descriptorAllocator.allocate(device, _materialLayout);


  writer.clear();
  writer.writeBuffer(
    0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  writer.writeImage(
    1,
    resources.colorImage.imageView,
    resources.colorSampler,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
  writer.writeImage(
    2,
    resources.metalRoughImage.imageView,
    resources.metalRoughSampler,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

  writer.updateSet(device, matData.materialSet);

  return matData;
}
