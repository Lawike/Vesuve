#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing_position_fetch : require

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
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

hitAttributeEXT vec2 attribs;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, scalar) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer{ 
	uint indices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
} PushConstants;

vec3 lcolor = sceneData.lightColor.xyz;
float lpow = sceneData.lightPower;
vec3 lpos = sceneData.lightPosition.xyz;

void main()
{
  uint triIndex0 = PushConstants.indexBuffer.indices[gl_PrimitiveID*3 + 0];
  uint triIndex1 = PushConstants.indexBuffer.indices[gl_PrimitiveID*3 + 1];
  uint triIndex2 = PushConstants.indexBuffer.indices[gl_PrimitiveID*3 + 2];

  Vertex vert0 = PushConstants.vertexBuffer.vertices[triIndex0];
  Vertex vert1 = PushConstants.vertexBuffer.vertices[triIndex1];
  Vertex vert2 = PushConstants.vertexBuffer.vertices[triIndex2];

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
  // Vector toward the light
  vec3  L;
  float lightIntensity = 20;
  float lightDistance  = 10000.0;
	
  vec3 lDir      = lpos - worldPos;
  lightDistance  = length(lDir);
  float LightDSquare = (lightDistance * lightDistance);
  if (LightDSquare != 0)
  {
    lightIntensity = lightIntensity / (lightDistance * lightDistance);
  }
  L = normalize(lDir);

  // Blinn phong
  float NdotL = clamp(dot(worldNormal, L), 0.0, 1.0);
  vec3 D = lpow * (lcolor.xyz * NdotL);
  vec3  specular    = vec3(0.,0.,0.);
  float attenuation = 1;
 if(dot(worldNormal, L) > 0)
  {
    float tMin   = 0.001;
    float tMax   = lightDistance;
    vec3  origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3  rayDir = L;
    uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    isShadowed   = true;
    traceRayEXT(topLevelAS,  // acceleration structure
                flags,       // rayFlags
                0xFF,        // cullMask
                0,           // sbtRecordOffset
                0,           // sbtRecordStride
                0,           // missIndex
                origin,      // ray origin
                tMin,        // ray min range
                rayDir,      // ray direction
                tMax,        // ray max range
                1            // payload (location = 1)
    );
    if(isShadowed)
    {
      attenuation = 0.3;
    }
    else {
      if (specular.x == 0 && specular.y == 0 && specular.z == 0)
      {
        specular = vec3(1,1,1);
      }
      const float kShininess = sceneData.shininess;
      const float kPi        = sceneData.specularCoefficient;
      const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
      vec3        V                   = normalize(-gl_WorldRayDirectionEXT);
      vec3        R                   = reflect(-L, worldNormal);
      float       S            = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);
      specular = specular * S;
    }
  }
  //
  // Normal debug
  const vec3 A =  sceneData.ambientCoefficient * sceneData.ambientColor.xyz;
  prd.hitValue = lightIntensity * attenuation * (D + specular + A);
}