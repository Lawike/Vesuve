#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif  // !GLM_ENABLE_EXPERIMENTAL


#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include "VkEngine.hpp"
#include "VkInitializers.hpp"
#include "VkLoader.hpp"
#include "VkTypes.hpp"
#include "stb_image.h"


// Add this helper function to compute world transforms for all nodes
void computeNodeWorldTransforms(const fastgltf::Asset& gltf, std::vector<glm::mat4>& worldTransforms)
{
  worldTransforms.resize(gltf.nodes.size());

  // Find root nodes (nodes that aren't children of any other node)
  std::vector<bool> isChild(gltf.nodes.size(), false);
  for (size_t i = 0; i < gltf.nodes.size(); ++i)
  {
    const auto& node = gltf.nodes[i];
    for (size_t childIndex : node.children)
    {
      if (childIndex < gltf.nodes.size())
      {
        isChild[childIndex] = true;
      }
    }
  }

  // Process root nodes recursively
  std::function<void(size_t, const glm::mat4&)> processNode = [&](size_t nodeIndex, const glm::mat4& parentTransform)
  {
    if (nodeIndex >= gltf.nodes.size())
      return;

    const auto& node = gltf.nodes[nodeIndex];
    glm::mat4 localTransform = glm::mat4(1.0f);

    // Extract local transform from the node
    std::visit(
      fastgltf::visitor{
        [&](const fastgltf::math::fmat4x4& matrix) { memcpy(&localTransform, matrix.data(), sizeof(matrix)); },
        [&](const fastgltf::TRS& transform)
        {
          glm::vec3 translation(transform.translation[0], transform.translation[1], transform.translation[2]);
          glm::quat rotation(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
          glm::vec3 scale(transform.scale[0], transform.scale[1], transform.scale[2]);

          glm::mat4 tm = glm::translate(glm::mat4(1.0f), translation);
          glm::mat4 rm = glm::toMat4(rotation);
          glm::mat4 sm = glm::scale(glm::mat4(1.0f), scale);

          localTransform = tm * rm * sm;
        }},
      node.transform);

    // Compute world transform
    worldTransforms[nodeIndex] = parentTransform * localTransform;

    // Process children
    for (size_t childIndex : node.children)
    {
      processNode(childIndex, worldTransforms[nodeIndex]);
    }
  };

  // Start processing from root nodes
  for (size_t i = 0; i < gltf.nodes.size(); ++i)
  {
    if (!isChild[i])
    {
      processNode(i, glm::mat4(1.0f));
    }
  }
}

//--------------------------------------------------------------------------------------------------
std::optional<std::shared_ptr<LoadedGLTF>> vkloader::loadGltf(VkEngine* engine, std::string_view filePath)
{
  fmt::print("Loading GLTF: {}", filePath);

  std::shared_ptr<LoadedGLTF> scene = std::make_shared<LoadedGLTF>();
  scene->creator = engine;
  LoadedGLTF& file = *scene.get();

  fastgltf::Parser parser{fastgltf::Extensions::KHR_materials_emissive_strength};

  constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                               fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
  // fastgltf::Options::LoadExternalImages;

  auto data = fastgltf::GltfDataBuffer::FromPath(filePath);

  fastgltf::Asset gltf;

  std::filesystem::path path = filePath;

  auto type = fastgltf::determineGltfFileType(data.get());
  if (type == fastgltf::GltfType::glTF)
  {
    auto load = parser.loadGltf(data.get(), path.parent_path(), gltfOptions);
    if (load)
    {
      gltf = std::move(load.get());
    }
    else
    {
      std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
      return {};
    }
  }
  else if (type == fastgltf::GltfType::GLB)
  {
    auto load = parser.loadGltfBinary(data.get(), path.parent_path(), gltfOptions);
    if (load)
    {
      gltf = std::move(load.get());
    }
    else
    {
      std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
      return {};
    }
  }
  else
  {
    std::cerr << "Failed to determine glTF container" << std::endl;
    return {};
  }
  // we can stimate the descriptors we will need accurately
  std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

  file.descriptorPool.init(engine->_device->getHandle().device, gltf.materials.size(), sizes);

  // load samplers
  for (fastgltf::Sampler& sampler : gltf.samplers)
  {
    VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
    sampl.maxLod = VK_LOD_CLAMP_NONE;
    sampl.minLod = 0;

    sampl.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
    sampl.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

    sampl.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

    VkSampler newSampler;
    vkCreateSampler(engine->_device->getHandle().device, &sampl, nullptr, &newSampler);

    file.samplers.push_back(newSampler);
  }

  // temporal arrays for all the objects to use while creating the GLTF data
  std::vector<std::shared_ptr<MeshAsset>> meshes;
  std::vector<std::shared_ptr<Node>> nodes;
  std::vector<std::unique_ptr<VulkanBackend::Image>> images;
  images.resize(gltf.images.size());
  std::vector<std::shared_ptr<GLTFMaterial>> materials;

  // load all textures
  int i = 0;
  for (fastgltf::Image& image : gltf.images)
  {
    // Need to transfer ownership
    loadImage(images.at(i), engine, gltf, image);

    if (images.at(i)->_handle.image != VK_NULL_HANDLE)
    {
      file.images[image.name.c_str()] = images.at(i)->_handle;
    }
    else
    {
      // we failed to load, so lets give the slot a default white texture to not
      // completely break loading
      images.at(i) = std::move(engine->_errorCheckerboardImage);
      std::cout << "gltf failed to load texture " << image.name << std::endl;
    }
    i++;
  }
  // create buffer to hold the material data
  file.materialDataBuffer = engine->createBuffer(
    sizeof(GLTFMetallicRoughness::MaterialConstants) * gltf.materials.size(),
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    VMA_MEMORY_USAGE_CPU_TO_GPU);
  int data_index = 0;

  VkBufferDeviceAddressInfo addressInfo = {};
  addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  addressInfo.pNext = nullptr;
  addressInfo.buffer = file.materialDataBuffer.buffer;
  VkDeviceAddress materialBufferAddress = vkGetBufferDeviceAddress(engine->_device->getHandle(), &addressInfo);
  file.materialDataBufferAddress = materialBufferAddress;

  GLTFMetallicRoughness::MaterialConstants* sceneMaterialConstants =
    (GLTFMetallicRoughness::MaterialConstants*)file.materialDataBuffer.info.pMappedData;

  for (fastgltf::Material& mat : gltf.materials)
  {
    std::shared_ptr<GLTFMaterial> newMat = std::make_shared<GLTFMaterial>();
    materials.push_back(newMat);
    file.materials[mat.name.c_str()] = newMat;

    GLTFMetallicRoughness::MaterialConstants constants;
    constants.colorFactors.x = mat.pbrData.baseColorFactor[0];
    constants.colorFactors.y = mat.pbrData.baseColorFactor[1];
    constants.colorFactors.z = mat.pbrData.baseColorFactor[2];
    constants.colorFactors.w = mat.pbrData.baseColorFactor[3];

    constants.metalRoughFactors.x = mat.pbrData.metallicFactor;
    constants.metalRoughFactors.y = mat.pbrData.roughnessFactor;
    constants.transparency = 1;
    constants.emissiveFactors.x = mat.emissiveFactor[0];
    constants.emissiveFactors.y = mat.emissiveFactor[1];
    constants.emissiveFactors.z = mat.emissiveFactor[2];
    constants.emissivePower = mat.emissiveStrength;
    // write material parameters to buffer

    MaterialPass passType = MaterialPass::MainColor;
    if (mat.alphaMode == fastgltf::AlphaMode::Blend)
    {
      passType = MaterialPass::Transparent;
      constants.transparency = 0.5;
    }
    sceneMaterialConstants[data_index] = constants;

    GLTFMetallicRoughness::MaterialResources materialResources;
    // default the material textures
    materialResources.colorImage = engine->_whiteImage->_handle;
    materialResources.colorSampler = engine->_defaultSamplerLinear;
    materialResources.metalRoughImage = engine->_whiteImage->_handle;
    materialResources.metalRoughSampler = engine->_defaultSamplerLinear;

    // set the uniform buffer for the material data
    materialResources.dataBuffer = file.materialDataBuffer.buffer;
    materialResources.dataBufferOffset = data_index * sizeof(GLTFMetallicRoughness::MaterialConstants);
    // grab textures from gltf file
    if (mat.pbrData.baseColorTexture.has_value())
    {
      size_t img = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
      size_t sampler = gltf.textures[mat.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

      materialResources.colorImage = images[img]->_handle;
      materialResources.colorSampler = file.samplers[sampler];
    }
    // build material
    newMat->data = engine->_metalRoughMaterial.writeMaterial(
      engine->_device->getHandle().device, passType, materialResources, file.descriptorPool);

    data_index++;
  }

  // use the same vectors for all meshes so that the memory doesnt reallocate as
  // often
  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;

  // Add this before processing meshes
  std::vector<glm::mat4> nodeWorldTransforms;
  computeNodeWorldTransforms(gltf, nodeWorldTransforms);

  // Create a mapping from mesh index to the nodes that use it
  std::vector<std::vector<size_t>> meshToNodes(gltf.meshes.size());
  for (size_t nodeIndex = 0; nodeIndex < gltf.nodes.size(); ++nodeIndex)
  {
    const auto& node = gltf.nodes[nodeIndex];
    if (node.meshIndex.has_value())
    {
      meshToNodes[*node.meshIndex].push_back(nodeIndex);
    }
  }

  for (size_t meshIndex = 0; meshIndex < gltf.meshes.size(); ++meshIndex)
  {
    fastgltf::Mesh& mesh = gltf.meshes[meshIndex];
    std::shared_ptr<MeshAsset> newmesh = std::make_shared<MeshAsset>();
    meshes.push_back(newmesh);
    file.meshes[mesh.name.c_str()] = newmesh;
    newmesh->name = mesh.name;

    // clear the mesh arrays each mesh, we dont want to merge them by error
    indices.clear();
    vertices.clear();

    for (auto&& p : mesh.primitives)
    {
      GeoSurface newSurface;
      bool isEmissive = false;
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initial_vtx = vertices.size();

      // load materials
      uint32_t matIndex = 0;
      if (p.materialIndex.has_value())
      {
        newSurface.material = materials[p.materialIndex.value()];
        matIndex = p.materialIndex.value();
        GLTFMetallicRoughness::MaterialConstants matp = sceneMaterialConstants[p.materialIndex.value()];
        isEmissive = matp.emissiveFactors.r + matp.emissiveFactors.g + matp.emissiveFactors.b > 0;
      }
      else
      {
        newSurface.material = materials[0];
      }

      // load indices
      {
        fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
          gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });

        uint32_t triangleCount = (uint32_t)indices.size() / 3;
        for (uint32_t t = 0; t < triangleCount; t++)
        {
          newmesh->materialIndices.push_back(matIndex);
        }
      }

      // load vertex positions
      {
        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
          gltf,
          posAccessor,
          [&](glm::vec3 v, size_t index)
          {
            Vertex newvtx;
            newvtx.position = v;
            newvtx.normal = {1, 0, 0};
            newvtx.color = glm::vec4{1.f};
            newvtx.uv_x = 0;
            newvtx.uv_y = 0;
            vertices[initial_vtx + index] = newvtx;
          });
      }

      // load vertex normals
      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end())
      {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
          gltf,
          gltf.accessors[(*normals).accessorIndex],
          [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].normal = v; });
      }

      // load UVs
      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end())
      {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
          gltf,
          gltf.accessors[(*uv).accessorIndex],
          [&](glm::vec2 v, size_t index)
          {
            vertices[initial_vtx + index].uv_x = v.x;
            vertices[initial_vtx + index].uv_y = v.y;
          });
      }

      // load vertex colors
      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end())
      {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
          gltf,
          gltf.accessors[(*colors).accessorIndex],
          [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].color = v; });
      }

      glm::vec3 minpos = vertices[initial_vtx].position;
      glm::vec3 maxpos = vertices[initial_vtx].position;
      for (int i = initial_vtx; i < vertices.size(); i++)
      {
        minpos = glm::min(minpos, vertices[i].position);
        maxpos = glm::max(maxpos, vertices[i].position);
      }

      newSurface.bounds.origin = (maxpos + minpos) / 2.f;
      newSurface.bounds.extents = (maxpos - minpos) / 2.f;
      newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);
      newmesh->surfaces.push_back(newSurface);

      if (isEmissive)
      {
        for (size_t nodeIndex : meshToNodes[meshIndex])
        {
          const glm::mat4& worldTransform = nodeWorldTransforms[nodeIndex];
          const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldTransform)));
          for (uint32_t i = 0; i < newSurface.count; i += 3)
          {
            uint32_t i0 = indices[newSurface.startIndex + i];
            uint32_t i1 = indices[newSurface.startIndex + i + 1];
            uint32_t i2 = indices[newSurface.startIndex + i + 2];

            glm::vec3 v0 = glm::vec3(worldTransform * glm::vec4(vertices[i0].position, 1.0));
            glm::vec3 v1 = glm::vec3(worldTransform * glm::vec4(vertices[i1].position, 1.0));
            glm::vec3 v2 = glm::vec3(worldTransform * glm::vec4(vertices[i2].position, 1.0));

            glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
            float area = 0.5f * glm::length(glm::cross(v1 - v0, v2 - v0));

            glm::vec3 emissionColor = {
              sceneMaterialConstants[matIndex].emissiveFactors.x,
              sceneMaterialConstants[matIndex].emissiveFactors.y,
              sceneMaterialConstants[matIndex].emissiveFactors.z};
            float strength = sceneMaterialConstants[matIndex].emissivePower;

            EmissiveTriangle tri;
            tri.x0 = glm::vec4(v0, 1);
            tri.x1 = glm::vec4(v1, 1);
            tri.x2 = glm::vec4(v2, 1);

            tri.normal = glm::vec4(normal.x, normal.y, normal.z, 1);
            tri.emission = glm::vec4(emissionColor.x, emissionColor.y, emissionColor.z, 1) * strength;
            tri.area = area;
            tri.extra[0] = 0;
            tri.extra[1] = 0;
            // Laisse tri.cdf vide pour l'instant, on le calcule après

            scene->emissiveTriangles.push_back(tri);  // <-- Ajoute à ta scène
          }
        }
      }
    }

    newmesh->meshBuffers = engine->uploadMesh(indices, vertices, newmesh->materialIndices, newmesh->name);
  }
  // load all nodes and their meshes
  for (fastgltf::Node& node : gltf.nodes)
  {
    std::shared_ptr<Node> newNode;

    // find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
    if (node.meshIndex.has_value())
    {
      newNode = std::make_shared<MeshNode>();
      static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
    }
    else
    {
      newNode = std::make_shared<Node>();
    }

    nodes.push_back(newNode);

    std::visit(
      fastgltf::visitor{
        [&](fastgltf::math::fmat4x4 matrix) { memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix)); },
        [&](fastgltf::TRS transform)
        {
          glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
          glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
          glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

          glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
          glm::mat4 rm = glm::toMat4(rot);
          glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

          newNode->localTransform = tm * rm * sm;
        }},
      node.transform);

    file.nodes[node.name.c_str()] = newNode;
  }

  // run loop again to setup transform hierarchy
  for (int i = 0; i < gltf.nodes.size(); i++)
  {
    fastgltf::Node& node = gltf.nodes[i];
    std::shared_ptr<Node>& sceneNode = file.nodes[node.name.c_str()];

    for (auto& c : node.children)
    {
      nodes[c]->parent = sceneNode;
      sceneNode->children.push_back(nodes[c]);
    }
  }

  // find the top nodes, with no parents
  for (auto& node : file.nodes)
  {
    if (node.second->parent.lock() == nullptr)
    {
      file.topNodes.push_back(node.second);
      node.second->refreshTransform(glm::mat4{1.f});
    }
  }

  scene->buildCDF();

  return scene;
}

//--------------------------------------------------------------------------------------------------
VkFilter vkloader::extractFilter(fastgltf::Filter filter)
{
  switch (filter)
  {
    // nearest samplers
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::NearestMipMapLinear: return VK_FILTER_NEAREST;

    // linear samplers
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest:
    case fastgltf::Filter::LinearMipMapLinear:
    default: return VK_FILTER_LINEAR;
  }
}

//--------------------------------------------------------------------------------------------------
VkSamplerMipmapMode vkloader::extractMipmapMode(fastgltf::Filter filter)
{
  switch (filter)
  {
    case fastgltf::Filter::NearestMipMapNearest:
    case fastgltf::Filter::LinearMipMapNearest: return VK_SAMPLER_MIPMAP_MODE_NEAREST;

    case fastgltf::Filter::NearestMipMapLinear:
    case fastgltf::Filter::LinearMipMapLinear:
    default: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  }
}

std::optional<AllocatedImage> vkloader::loadImage(
  std::unique_ptr<VulkanBackend::Image>& image,
  VkEngine* engine,
  fastgltf::Asset& asset,
  fastgltf::Image& gltfImage)
{
  AllocatedImage newImage{};

  int width, height, nrChannels;

  std::visit(
    fastgltf::visitor{
      [](auto& arg) {},
      [&](fastgltf::sources::URI& filePath)
      {
        assert(filePath.fileByteOffset == 0);  // We don't support offsets with stbi.
        assert(filePath.uri.isLocalPath());    // We're only capable of loading
                                               // local files.

        const std::string path(filePath.uri.path().begin(),
                               filePath.uri.path().end());  // Thanks C++.
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
        if (data)
        {
          VkExtent3D imagesize;
          imagesize.width = width;
          imagesize.height = height;
          imagesize.depth = 1;

          engine->createImage(image, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

          stbi_image_free(data);
        }
      },
      [&](fastgltf::sources::Vector& vector)
      {
        unsigned char* data = stbi_load_from_memory(
          reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
          static_cast<int>(vector.bytes.size()),
          &width,
          &height,
          &nrChannels,
          4);
        if (data)
        {
          VkExtent3D imagesize;
          imagesize.width = width;
          imagesize.height = height;
          imagesize.depth = 1;

          engine->createImage(image, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

          stbi_image_free(data);
        }
      },
      [&](fastgltf::sources::Array& array)
      {
        unsigned char* data = stbi_load_from_memory(
          reinterpret_cast<const stbi_uc*>(array.bytes.data()),
          static_cast<int>(array.bytes.size()),
          &width,
          &height,
          &nrChannels,
          4);
        if (data)
        {
          VkExtent3D imagesize;
          imagesize.width = width;
          imagesize.height = height;
          imagesize.depth = 1;

          engine->createImage(image, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

          stbi_image_free(data);
        }
      },
      [&](fastgltf::sources::BufferView& view)
      {
        auto& bufferView = asset.bufferViews[view.bufferViewIndex];
        auto& buffer = asset.buffers[bufferView.bufferIndex];

        std::visit(
          fastgltf::visitor{// We only care about VectorWithMime here, because we
                            // specify LoadExternalBuffers, meaning all buffers
                            // are already loaded into a vector.
                            [](auto& arg) {},
                            [&](fastgltf::sources::Array& array)
                            {
                              unsigned char* data = stbi_load_from_memory(
                                reinterpret_cast<const stbi_uc*>(array.bytes.data()) + bufferView.byteOffset,
                                static_cast<int>(bufferView.byteLength),
                                &width,
                                &height,
                                &nrChannels,
                                4);
                              if (data)
                              {
                                VkExtent3D imagesize;
                                imagesize.width = width;
                                imagesize.height = height;
                                imagesize.depth = 1;

                                engine->createImage(
                                  image, data, imagesize, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);

                                stbi_image_free(data);
                              }
                            }},
          buffer.data);
      },
    },
    gltfImage.data);

  // if any of the attempts to load the data failed, we havent written the image
  // so handle is null
  if (image->_handle.image == VK_NULL_HANDLE)
  {
    const char* test = stbi_failure_reason();
    return {};
  }
}

//--------------------------------------------------------------------------------------------------
void LoadedGLTF::Draw(const glm::mat4& topMatrix, DrawContext& ctx)
{
  // create renderables from the scenenodes
  for (auto& n : topNodes)
  {
    n->Draw(topMatrix, ctx);
  }
}

//--------------------------------------------------------------------------------------------------
void LoadedGLTF::clearAll()
{
  VkDevice dv = creator->_device->getHandle().device;

  descriptorPool.destroyPools(dv);
  creator->destroyBuffer(materialDataBuffer);

  for (auto& [k, v] : meshes)
  {
    creator->destroyBuffer(v->meshBuffers.indexBuffer);
    creator->destroyBuffer(v->meshBuffers.vertexBuffer);
    creator->destroyBuffer(v->meshBuffers.materialIndicesBuffer);
  }

  for (auto& [k, v] : images)
  {
    if (v.image == creator->_errorCheckerboardImage->_handle.image)
    {
      //dont destroy the default images
      continue;
    }
    creator->destroyImage(v);
  }

  for (auto& sampler : samplers)
  {
    vkDestroySampler(dv, sampler, nullptr);
  }
}

void LoadedGLTF::buildCDF()
{
  float total = 0.0f;

  for (auto& tri : this->emissiveTriangles)
  {
    float luminance =
      glm::dot(glm::vec3(tri.emission), glm::vec3(0.2126f, 0.7152f, 0.0722f));  // ITU-R BT.709 standard luminance coef
    tri.importance = tri.area * luminance;
    total += tri.importance;
  }

  for (auto& tri : this->emissiveTriangles)
  {
    tri.importance /= total;
  }
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> vkloader::loadGltfMeshes(
  VkEngine* engine,
  std::filesystem::path filePath)
{
  //> openmesh
  std::cout << "Loading GLTF: " << filePath << std::endl;

  auto data = fastgltf::GltfDataBuffer::FromPath(filePath);

  constexpr auto gltfOptions = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

  fastgltf::Asset gltf;
  fastgltf::Parser parser{};

  auto load = parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);
  if (load)
  {
    gltf = std::move(load.get());
  }
  else
  {
    fmt::print("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
    return {};
  }
  std::vector<std::shared_ptr<MeshAsset>> meshes;

  // use the same vectors for all meshes so that the memory doesnt reallocate as
  // often
  std::vector<uint32_t> indices;
  std::vector<Vertex> vertices;
  for (fastgltf::Mesh& mesh : gltf.meshes)
  {
    MeshAsset newmesh;

    newmesh.name = mesh.name;

    // clear the mesh arrays each mesh, we dont want to merge them by error
    indices.clear();
    vertices.clear();

    for (auto&& p : mesh.primitives)
    {
      GeoSurface newSurface;
      newSurface.startIndex = (uint32_t)indices.size();
      newSurface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

      size_t initial_vtx = vertices.size();

      // load indexes
      {
        fastgltf::Accessor& indexaccessor = gltf.accessors[p.indicesAccessor.value()];
        indices.reserve(indices.size() + indexaccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(
          gltf, indexaccessor, [&](std::uint32_t idx) { indices.push_back(idx + initial_vtx); });
      }

      // load vertex positions
      {
        fastgltf::Accessor& posAccessor = gltf.accessors[p.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + posAccessor.count);

        fastgltf::iterateAccessorWithIndex<glm::vec3>(
          gltf,
          posAccessor,
          [&](glm::vec3 v, size_t index)
          {
            Vertex newvtx;
            newvtx.position = v;
            newvtx.normal = {1, 0, 0};
            newvtx.color = glm::vec4{1.f};
            newvtx.uv_x = 0;
            newvtx.uv_y = 0;
            vertices[initial_vtx + index] = newvtx;
          });
      }

      // load vertex normals
      auto normals = p.findAttribute("NORMAL");
      if (normals != p.attributes.end())
      {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
          gltf,
          gltf.accessors[(*normals).accessorIndex],
          [&](glm::vec3 v, size_t index) { vertices[initial_vtx + index].normal = v; });
      }

      // load UVs
      auto uv = p.findAttribute("TEXCOORD_0");
      if (uv != p.attributes.end())
      {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
          gltf,
          gltf.accessors[(*uv).accessorIndex],
          [&](glm::vec2 v, size_t index)
          {
            vertices[initial_vtx + index].uv_x = v.x;
            vertices[initial_vtx + index].uv_y = v.y;
          });
      }

      // load vertex colors
      auto colors = p.findAttribute("COLOR_0");
      if (colors != p.attributes.end())
      {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
          gltf,
          gltf.accessors[(*colors).accessorIndex],
          [&](glm::vec4 v, size_t index) { vertices[initial_vtx + index].color = v; });
      }
      newmesh.surfaces.push_back(newSurface);
    }

    // display the vertex normals
    constexpr bool OverrideColors = true;
    if (OverrideColors)
    {
      for (Vertex& vtx : vertices)
      {
        vtx.color = glm::vec4(vtx.normal, 1.f);
      }
    }
    newmesh.meshBuffers = engine->uploadMesh(indices, vertices, newmesh.materialIndices, newmesh.name);

    meshes.emplace_back(std::make_shared<MeshAsset>(std::move(newmesh)));
  }

  return meshes;
}
