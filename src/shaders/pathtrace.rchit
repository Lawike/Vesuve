#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing_position_fetch : require

#include "raycommon.glsl"
#include "random.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT shadowPayload prdShadow;
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 1, binding = 0) uniform SceneData
{
	mat4 view;
	mat4 invView;
	mat4 proj;
	mat4 invProj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 cameraPosition;
	vec4 lightPosition;
	vec4 lightColor;
	float lightPower;
	float specularCoefficient;
	float ambientCoefficient;
	float shininess;
	float screenGamma;
	float aspectRatio;
    uint frameIndex;
} sceneData;

layout(set = 0, binding = 2, scalar) buffer GPUInstanceBuffers_ { GPUInstanceBuffers buffersAdresses[]; } instanceBuffers;

hitAttributeEXT vec2 attribs;

layout(buffer_reference, scalar) readonly buffer Vertices{ 
	Vertex v[];
};

layout(buffer_reference, scalar) readonly buffer Indices{ 
	uint i[];
};

layout(buffer_reference, scalar) readonly buffer Materials {
    Material m[];
};

layout(buffer_reference, scalar) readonly buffer MaterialIndices {
    uint i[];
};

vec3 lcolor = sceneData.lightColor.xyz;
float lpow = sceneData.lightPower;
vec3 lpos = sceneData.lightPosition.xyz;

void main()
{
  GPUInstanceBuffers instance = instanceBuffers.buffersAdresses[gl_InstanceCustomIndexEXT];
  Vertices vertices = Vertices(instance.vertexBufferAddress);
  Indices indices = Indices(instance.indexBufferAddress);
  Materials materials = Materials(instance.materialBufferAddress);
  MaterialIndices materialIndices = MaterialIndices(instance.materialIndicesBufferAddress);

  uint matIndex = materialIndices.i[gl_PrimitiveID];
  Material mat = materials.m[matIndex];

  uint triIndex0 = indices.i[gl_PrimitiveID*3 + 0];
  uint triIndex1 = indices.i[gl_PrimitiveID*3 + 1];
  uint triIndex2 = indices.i[gl_PrimitiveID*3 + 2];

  Vertex vert0 = vertices.v[triIndex0];
  Vertex vert1 = vertices.v[triIndex1];
  Vertex vert2 = vertices.v[triIndex2];

  vec3 v0 = vert0.position;
  vec3 v1 = vert1.position;
  vec3 v2 = vert2.position;

  vec3 n0 = vert0.normal;
  vec3 n1 = vert1.normal;
  vec3 n2 = vert2.normal;

  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // Computing the coordinates of the hit position
  //vec3 geoNormal = cross(v1 - v0, v2 - v0);
  vec3 normal = n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z;

  const vec3 position = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  // Transforming the position to world space

  // Computing the normal at hit position
  const vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to world space
  vec3 emittance = mat.emissiveFactors * mat.emissivePower;

  // Pick a random direction from here and keep going.
  vec3 tangent, bitangent;
  createCoordinateSystem(worldNormal, tangent, bitangent);
  vec3 rayOrigin    = worldPos;
  vec3 rayDirection = samplingHemisphere(prd.seed, tangent, bitangent, worldNormal);
  const float cos_theta = dot(rayDirection, worldNormal);
  // Probability density function of samplingHemisphere choosing this rayDirection
  const float p = cos_theta / M_PI;
  // Compute the BRDF for this ray (assuming Lambertian reflection)
  vec3 albedo = mat.colorFactors.xyz;
  vec3 BRDF = albedo / M_PI;

  prd.rayOrigin    = rayOrigin;
  prd.rayDir = rayDirection;
  prd.hitValue     = emittance;
  prd.weight       = BRDF * cos_theta / p;
}