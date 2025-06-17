#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_ray_tracing_position_fetch : require

#include "dysneyBRDF.glsl"

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

layout(buffer_reference, scalar) readonly buffer EmissiveTriangles {
    EmissiveTriangle e[];
};

layout( push_constant ) uniform constants
{
	vec4 clearColor;
	uint emissiveTrianglesCount;
} PushConstants;

vec3 lcolor = sceneData.lightColor.xyz;
float lpow = sceneData.lightPower;
vec3 lpos = sceneData.lightPosition.xyz;
vec3 camPos = sceneData.cameraPosition.xyz;

int M = 32;

float luminance(float r, float g, float b) {
	return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// Ldirect(x) = Li(x,l) * Fr(x, w_i, w_o) * cos(theta_i) * G / p(l)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 directLighting(
	vec3 worldPos, vec3 lightPos, vec3 camPos, vec3 normal, vec3 lightNormal, bool useLightNormal,
	vec3 emission, vec3 albedo, float roughness, float metallic
) {
	vec3 toLight = lightPos - worldPos;
	float distSquared = dot(toLight, toLight);
	float dist = sqrt(distSquared);

	vec3 lightDir = toLight / dist;

	vec3 viewDir = normalize(camPos - worldPos);
	vec3 halfVec = normalize(lightDir + viewDir);

	// Cosines
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    float NdotH = max(dot(normal, halfVec), 0.0);
    float LdotH = max(dot(lightDir, halfVec), 0.0);
    float LdotN_light = max(dot(-lightDir, lightNormal), 0.0); // light's cosine

	// Visibility test (shadow ray)
    /**if (!isVisible(worldPosition, lightPosition)) {
        return vec3(0.0);
    }*/

	// Geometry term (area-based solid angle)
    float G = (LdotN_light * NdotL) / max(distSquared, 0.001);

	// Cook-Torrance BRDF
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F = fresnelSchlick(LdotH, F0);

	float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

	float D = alpha2 / (M_PI * pow((NdotH * NdotH * (alpha2 - 1.0) + 1.0), 2.0));

	float k = alpha / 2.0;
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G_Smith = G1V * G1L;

	vec3 specular = (D * F * G_Smith) / (4.0 * max(NdotV * NdotL, 0.001));

	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

	vec3 diffuse = (albedo / M_PI);

    // Final contribution
    vec3 brdf = kD * diffuse + specular;
    vec3 radiance = emission;

    vec3 contribution = brdf * radiance * G;

    return contribution;
}

vec3 sampleTrianglePoint(in vec3 x0, in vec3 x1, in vec3 x2, inout uint seed)
{
	float a = rnd(seed);
	float b = rnd(seed);
	return x0 + a * ( x1 - x0 ) + b * ( x2 - x0);
}

void main()
{
  GPUInstanceBuffers instance = instanceBuffers.buffersAdresses[gl_InstanceCustomIndexEXT];
  Vertices vertices = Vertices(instance.vertexBufferAddress);
  Indices indices = Indices(instance.indexBufferAddress);
  Materials materials = Materials(instance.materialBufferAddress);
  MaterialIndices materialIndices = MaterialIndices(instance.materialIndicesBufferAddress);
  EmissiveTriangles triBuf = EmissiveTriangles(instance.emissiveTrianglesBufferAddress);

  uint matIndex = materialIndices.i[gl_PrimitiveID];
  Material mat = materials.m[matIndex];

  // Get triangle vertices
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
  // Interpolate position and normal
  vec3 normal = n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z;
  const vec3 position = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
  const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));  // Transforming the position to world space

  // Computing the normal at hit position
  const vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to world space

  // Material properties
  vec3 albedo = mat.colorFactors.xyz;
  float metallic = mat.metalRoughFactors.x;
  float roughness = mat.metalRoughFactors.y;
  vec3 emittance = mat.emissiveFactors * mat.emissivePower;

  // Set emitted light
  prd.hitValue = emittance;

  // Determine material type and handle accordingly
  bool isEmissive = dot(emittance, emittance) > 0.0;
  bool isMirror = (metallic > 0.9 && roughness < 0.1);

  if (isEmissive && prd.depth == 0) {
    // Direct hit on light source - terminate path
    prd.done = 1;
    return;
  }
  if (isMirror) {
    // Perfect mirror reflection
    vec3 rayDir = reflect(gl_WorldRayDirectionEXT, worldNormal);
        
    prd.rayOrigin = worldPos;
    prd.rayDir    = rayDir;
    prd.weight    = albedo;
    prd.done      = 0;
  } else {
	// Lambertian diffuse reflection

	// Pick a random direction from here and keep going.
	vec3 tangent, bitangent;
	createCoordinateSystem(worldNormal, tangent, bitangent);
	vec3 rayOrigin    = worldPos;
	vec3 rayDirection = samplingHemisphere(prd.seed, tangent, bitangent, worldNormal);
	const float cos_theta = max(dot(worldNormal, rayDirection), 0.0);
	float p = cos_theta / M_PI;
	vec3 albedo = mat.colorFactors.xyz;
	vec3 BRDF = albedo / M_PI;

	uint lightCount = PushConstants.emissiveTrianglesCount;
	
	if (lightCount > 0 ) {

		uint lightToSample = min(uint(rnd(prd.seed) * float(lightCount)), lightCount - 1);
		EmissiveTriangle tri = triBuf.e[lightToSample];
		float emissionLum = luminance(tri.emission.r, tri.emission.g, tri.emission.b); 

		// Sample triangle point
		vec3 triPoint = sampleTrianglePoint(tri.x0.xyz, tri.x1.xyz, tri.x2.xyz, prd.seed);

		vec3 p_hat = directLighting(worldPos, triPoint, rayOrigin, worldNormal, tri.normal.xyz, false, tri.emission.xyz, albedo, roughness, metallic);
		prd.hitValue = emittance;

		float wl = (1.0f/float(lightCount)) / ((1.0f/float(lightCount)) + p);
		float wp = p / ((1.0f/float(lightCount)) + p);

		prd.weight = p_hat / wl + (BRDF * cos_theta / wp);

	} else {
		prd.hitValue     = emittance;
		prd.weight       = BRDF * cos_theta / p;
	}
    /**
	PathToLight path;
	path.prev = rayOrigin;
	path.hit = worldPos;
	path.next = tri;
	float DIweight = 0.0;
	float emissionLum = luminance(tri.emission.r, tri.emission.g, tri.emission.b); 

	float p_hat = evaluatePHat(worldPos, tri.center.xyz, camPos, worldNormal, tri.normal.xyz, false, tri.importance, emissionLum, roughness, metallic);

	if (isnan(p_hat)) {
		p_hat = 1.0f;

	/**
	// Probability density function of samplingHemisphere choosing this rayDirection
	int M = 42;
	Reservoir r;
	r.WSum = 0;

   float albedoLum = luminance(albedo.r, albedo.g, albedo.b);

	// Sample light and add to reservoir
	for (int i = 0; i < M; i++) 
	{


		PathToLight path;
		path.prev = rayOrigin;
		path.hit = worldPos;
		path.next = tri;
		float p = 1.0 / float(lightCount);
		float weight = (1.0/42.0) * evaluatePHat(path.hit, tri.center.xyz, camPos, worldNormal, tri.normal.xyz, false, albedoLum, tri.importance, roughness, metallic) / p;
		addSample(path, weight, r, prd.seed);
	}
	// Compute the BRDF for this ray (assuming Lambertian reflection)

	PathToLight path = r.sampleOut;
	EmissiveTriangle tri = path.next;

	float finalWeight = 1.0 / evaluatePHat(path.hit, tri.center.xyz, camPos, worldNormal, tri.normal.xyz, false, albedoLum, tri.importance, roughness, metallic) * r.WSum;
	*/


	/**
	prd.hitValue     = tri.emission.rgb;
	prd.weight       = finalWeight;
	*/
	prd.rayOrigin    = rayOrigin;
	prd.rayDir       = rayDirection;
  }
}