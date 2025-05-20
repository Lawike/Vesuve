#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing_position_fetch : require

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
};

hitAttributeEXT vec2 attribs;

vec3 lcolor = vec3(1.0,0.0,0.0);
float lpow = 5.0;
vec3 lpos = vec3(10,10,10);

void main()
{
  vec3 v0 = gl_HitTriangleVertexPositionsEXT[0];
  vec3 v1 = gl_HitTriangleVertexPositionsEXT[1];
  vec3 v2 = gl_HitTriangleVertexPositionsEXT[2];
	
  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // Computing the coordinates of the hit position
  vec3 geoNormal = cross(v1 - v0, v2 - v0);
  const vec3 position = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  // Transforming the position to world space

  // Computing the normal at hit position
  const vec3 worldNormal = normalize(vec3(geoNormal * gl_WorldToObjectEXT));  // Transforming the normal to world space
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
  vec3  specular    = vec3(0);
  float attenuation = 1;
 /** if(dot(worldNormal, L) > 0)
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
    // TODO Specular lighting
    }
  }**/
  //
  // Normal debug
  prd.hitValue = D * lightIntensity;
}