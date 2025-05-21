#version 450

#extension GL_GOOGLE_include_directive : enable
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPos;

layout (location = 0) out vec4 outFragColor;

vec4 ambientColor = sceneData.ambientColor;
vec4 diffuseColor = vec4(0.0, 0.0, 0.0, 1.0);
vec4 specColor = vec4(0.0, 0.0, 0.0, 1.0);
float shininess = sceneData.shininess;
float screenGamma = sceneData.screenGamma; // Assume the monitor is calibrated to the sRGB color space
float ambientCoefficient = sceneData.ambientCoefficient;
float specularCoefficient = sceneData.specularCoefficient;

// test

vec3 campos = sceneData.cameraPosition.xyz;
vec3 lpos = sceneData.lightPosition.xyz;
vec4 lcolor = sceneData.lightColor;
float lpow = sceneData.lightPower;

void main() 
{
	// Blinn phong model equation
	// N
	vec3 normal = normalize(inNormal);
	// L
	vec3 lightDir = lpos - inPos;
	// V
	vec3 viewDir = campos - inPos;
	// H
	vec3 halfAngle = normalize(viewDir + lightDir);
	float NdotL = clamp(dot(normal, lightDir), 0.0, 1.0);
    float NdotH = clamp(dot(normal, halfAngle), 0.0, 1.0);
	vec4 D = lpow/10 * (lcolor * NdotL);
	float specularHighlight = pow(NdotH, shininess);
	vec4 S = specularCoefficient * (lcolor * specularHighlight);

	diffuseColor +=  D;
	specColor += S;

	vec4 ambience = ambientCoefficient * ambientColor;
	vec4 BlinnPhong = ambience + diffuseColor + specColor;
	outFragColor = BlinnPhong;
	// For debug
	// outFragColor = vec4(inPos, 1);
}

