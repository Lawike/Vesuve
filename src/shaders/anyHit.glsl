//#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : enable

#include "random.glsl"
#include "raycommon.glsl"

#ifdef PAYLOAD_0
layout(location = 0) rayPayloadInEXT hitPayload prd;
#elif defined(PAYLOAD_1)
layout(location = 1) rayPayloadInEXT shadowPayload prd;
#endif

layout(buffer_reference, scalar) readonly buffer Vertices{ 
	Vertex v[];
};

layout(buffer_reference, scalar) readonly buffer Indices{ 
	uint i[];
};

layout(buffer_reference, scalar) readonly buffer Materials {
    Material m[];
};
layout(set = 0, binding = 2, scalar) buffer GPUInstanceBuffers_ { GPUInstanceBuffers buffersAdresses[]; } instanceBuffers;

void main()
{
  GPUInstanceBuffers instance = instanceBuffers.buffersAdresses[gl_InstanceCustomIndexEXT];
  Materials materials = Materials(instance.materialBufferAddress);

  Material mat = materials.m[0];
  if (mat.transparency == 0)
    ignoreIntersectionEXT;
  else if(rnd(prd.seed) > mat.transparency)
    ignoreIntersectionEXT;
}