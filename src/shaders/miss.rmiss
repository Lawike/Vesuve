#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing : require
#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
	if(prd.depth == 0)
	{
	  prd.hitValue = vec3(1,1,1) * 0.8;
	}
	else {
	  prd.hitValue = vec3(0.01);  // Tiny contribution from environment
	}
	// Ending trace
	prd.depth = 100;    
}