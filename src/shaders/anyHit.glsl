#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing_position_fetch : require

#include "random.glsl"
#include "raycommon.glsl"

#ifdef PAYLOAD_0
layout(location = 0) rayPayloadInEXT hitPayload prd;
#elif defined(PAYLOAD_1)
layout(location = 1) rayPayloadInEXT shadowPayload prd;
#endif

layout(buffer_reference, scalar) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(buffer_reference, scalar) readonly buffer IndexBuffer{ 
	uint indices[];
};

layout(buffer_reference, scalar) readonly buffer MaterialBuffer {
    Material materials[];
};

//push constants block
layout( push_constant ) uniform constants
{
	VertexBuffer vertexBuffer;
	IndexBuffer indexBuffer;
	MaterialBuffer materialBuffer;
} PushConstants;

void main()
{
 Material mat = PushConstants.materialBuffer.materials[0];
 if (mat.transparency == 0)
    ignoreIntersectionEXT;
 else if(rnd(prd.seed) > mat.transparency)
    ignoreIntersectionEXT;
}