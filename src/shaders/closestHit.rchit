#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : enable

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};
layout(buffer_reference, std430) readonly buffer IndexBuffer{ 
	ivec3 indices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
} PushConstants;

vec3 lcolor = vec3(1.0,0.0,0.0);
float lpow = 5.0;

void main()
{
  ivec3 ind = PushConstants.indexBuffer.indices[gl_PrimitiveID];
  Vertex v0 = PushConstants.vertexBuffer.vertices[ind.x];
  Vertex v1 = PushConstants.vertexBuffer.vertices[ind.y];
  Vertex v2 = PushConstants.vertexBuffer.vertices[ind.z];
	
  const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

  // Computing the coordinates of the hit position
  const vec3 position      = v0.position * barycentrics.x + v1.position * barycentrics.y + v2.position * barycentrics.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  // Transforming the position to world space

  // Computing the normal at hit position
  const vec3 normal      = v0.normal * barycentrics.x + v1.normal * barycentrics.y + v2.normal * barycentrics.z;
  const vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to world space

  // Vector toward the light
  vec3  L;
  float lightIntensity = 20;
  float lightDistance  = 10000.0;
	
  vec3 lDir      = vec3(0.0,10.0,10.0) - worldPos;
  lightDistance  = length(lDir);
  lightIntensity = lightIntensity / (lightDistance * lightDistance);
  L              = normalize(lDir);

  float NdotL = clamp(dot(worldNormal, L), 0.0, 1.0);
  vec3 D = lpow * (lcolor.xyz * NdotL);
  hitValue = D * lightIntensity;
}