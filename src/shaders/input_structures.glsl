#version 460

layout(set = 0, binding = 0) uniform SceneData
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

/*
#ifdef USE_BINDLESS
layout(set = 0, binding = 1) uniform sampler2D allTextures[];
#else
*/
layout(set = 1, binding = 0) uniform GLTFMaterialData{   
	vec4 colorFactors;
	vec4 metal_rough_factors;
	int colorTexID;
	int metalRoughTexID;
} materialData;
layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
//#endif


