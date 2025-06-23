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

int M = 256;

float luminance(float r, float g, float b) {
	return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

float luminance(vec3 rgb) {
	return 0.2126f * rgb.r + 0.7152f * rgb.g + 0.0722f * rgb.b;
}


vec3 EvaluatePbrLighting(
	vec3 worldPos, vec3 lightDir, vec3 camPos, vec3 normal,
	vec3 emission, vec3 albedo, float roughness, float metallic, vec3 lightNormal, float dist
) {
		float dist2 =  dist*dist;
		
		// View vector (from hit to camera)
		vec3 V = normalize(camPos - worldPos);
		vec3 H = normalize(V + lightDir);

		// Cosines
		float NdotL = max(dot(normal, lightDir), 0.0);
		float NdotV = max(dot(normal, V), 0.0);

		// GGX / Cook-Torrance specular
        float rough2 = roughness * roughness;
        float NdotH = max(dot(normal, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // Normal Distribution (GGX)
        float alpha2 = rough2 * rough2;
        float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
        float D = alpha2 / max((M_PI * denom * denom), 1e-6);

        // Geometry (Smith-Schlick-GGX)
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
        float G_V = NdotV / (NdotV * (1.0 - k) + k);
        float G_L = NdotL / (NdotL * (1.0 - k) + k);
        float G = G_V * G_L;

        // Fresnel (Schlick)
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

        // Specular BRDF
        vec3 specular = (D * G * F) / (4.0 * NdotV * NdotL + 1e-6);

        // Diffuse (Lambert modulated by energy conservation)
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        vec3 diffuse = kD * (albedo / M_PI);

        // Total BRDF (diffuse + specular)
        vec3 BRDF = diffuse + specular;

        // Cosine at light
        float cosLight = max(dot(lightNormal, -lightDir), 0.0);

        // Accumulate light: include cosines and distance^2
        vec3 contrib = emission * BRDF * NdotL * cosLight / dist2;
        return contrib;
}

vec3 sampleTrianglePoint(in vec3 x0, in vec3 x1, in vec3 x2, inout uint seed)
{
	float r1 = rnd(seed);
	float r2 = rnd(seed);
	float sqrt_r1 = sqrt(r1);
	return (1.0 - sqrt_r1) * x0 + (sqrt_r1 * (1.0 - r2)) * x1 + (r2 * sqrt_r1) * x2;
}

bool visibilityTest(vec3 lightDir, float lightDistance, vec3 worldPos)
{
	float tMin   = 0.001;
	float tMax   = lightDistance;
	vec3  origin = worldPos;
	vec3  rayDir = lightDir;
	uint  flags  = gl_RayFlagsSkipClosestHitShaderEXT;
	prdShadow.isHit = true;
	prdShadow.seed  = prd.seed;
	traceRayEXT(topLevelAS,  // acceleration structure
				flags,       // rayFlags
				0xFF,        // cullMask
				0,           // sbtRecordOffset
				0,           // sbtRecordStride
				1,           // missIndex
				origin,      // ray origin
				tMin,        // ray min range
				rayDir,      // ray direction
				tMax,        // ray max range
				1            // payload (location = 1)
	);
	prd.seed = prdShadow.seed; 
	if(prdShadow.isHit)
	{
		return false;
	}
	return true;
}


vec3 lambertDiffuseLight(vec3 lightDir, vec3 lightNormal, float dist, vec3 baseColor, vec3 lightEmission, vec3 normal)
{
	float dist2 = dist * dist;
	float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuseBRDF = baseColor / M_PI;
    // Cosine of angle at the light surface
    float cosLight = max(dot(lightNormal, -lightDir), 0.0);
    // Geometry/attenuation: include cosines and distance^2 (area factor can be applied separately)
    vec3 contrib = lightEmission * diffuseBRDF * NdotL * cosLight / dist2;
    return contrib;
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
  const vec3 worldNormal = normalize(vec3(normal * gl_WorldToObjectEXT));  // Transforming the normal to object space

  // Material properties
  vec3 albedo = mat.colorFactors.xyz;
  float metallic = mat.metalRoughFactors.x;
  float roughness = mat.metalRoughFactors.y;
  vec3 emittance = mat.emissiveFactors * mat.emissivePower;

  // Set emitted light
  prd.hitValue  = emittance;
  prd.done      = 0;

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
    prd.attenuation *= mat.metalRoughFactors.x;    
    prd.rayOrigin = worldPos;
    prd.rayDir    = rayDir;
    prd.weight	  = albedo;
  } else {
	// Lambertian diffuse reflection

	// Pick a random direction from here and keep going.
	vec3 tangent, bitangent;
	createCoordinateSystem(worldNormal, tangent, bitangent);
	vec3 rayOrigin    = worldPos;
	// Sample rayDirection
	vec3 rayDirection = samplingHemisphere(prd.seed, tangent, bitangent, worldNormal);
	// Indirect contribution (ignored for now)
	float cos_theta = abs(dot(worldNormal, rayDirection));
	float pdf_dir = cos_theta / M_PI;
	vec3 albedo = mat.colorFactors.xyz;
	vec3 BRDF = albedo / M_PI;
	uint lightCount = PushConstants.emissiveTrianglesCount;
	
	Reservoir r;
	r.WSum = 0.f;
	for (int i = 0; i < M; i++) {
		// Sample light from all the emissive triangle of the scene
		uint lightToSample = min(uint(rnd(prd.seed) * float(lightCount)), lightCount - 1);
		EmissiveTriangle tri = triBuf.e[lightToSample];
		float pdf_light = 1.0f /  float(lightCount); 

		// Sample area light point (triangle sampling)
		vec3 triPoint = sampleTrianglePoint(tri.x0.xyz, tri.x1.xyz, tri.x2.xyz, prd.seed);
		vec3 lightDir = normalize(triPoint - worldPos);
		float dist = distance(worldPos, triPoint);
		float pdf_triangle = 1.0f / float(tri.area);

		// Evaluate direct lighting
		vec3 f_x = EvaluatePbrLighting(rayOrigin, lightDir, camPos, worldNormal, tri.emission.xyz, albedo, roughness, metallic, tri.normal.xyz, dist);
		
		// Use luminance as a proportionnal target function
		float dist2 = dist * dist;
		float cosLight = max(dot(tri.normal.xyz, -lightDir), 1e-6);
		float pdf_lum = dist2 / cosLight;

		// Generalized Balance heuristic
		float pdf = pdf_light * pdf_triangle * pdf_lum;

		float NdotL = max(dot(worldNormal, lightDir), 0.0);
		float contrib = dot(f_x, vec3(NdotL));

		float w =  luminance(f_x) * pdf / float(M);
		
		Sample path;
		path.origin = prd.rayOrigin;
		path.hit = worldPos;
		path.light = tri;
		path.lightPos = triPoint;
		path.bounceDir = rayDirection;
		addSample(path, w, r, prd.seed);
	}

	Sample y = r.sampleOut;

	// Evaluate lighting with visibility test
	float dist = distance(worldPos, y.lightPos);
	vec3 lightDir = normalize(y.lightPos - worldPos);
	bool visibility = true;
	vec3 f_y = vec3(0); //BRDF  ;
	float W = 1.0f;// cos_theta / pdf_dir;

	// Cosine of angle at hit point
	float NdotL = max(dot(worldNormal, lightDir), 0.0);
	if(NdotL > 0)
	{
		visibility = visibilityTest(lightDir, dist, worldPos);
	}
	if (visibility) {
		f_y += EvaluatePbrLighting(rayOrigin, lightDir, camPos, worldNormal, y.light.emission.xyz, albedo, roughness, metallic, y.light.normal.xyz, dist); // Direct contribution
		float p_y = max(luminance(f_y), 1e-6);
		W = (1.0f / p_y) * r.WSum;
	}
	vec3 indirectContribution = BRDF * cos_theta / pdf_dir;
	vec3 directContribution = f_y * W;

	prd.hitValue	 = emittance;
	prd.weight		 = directContribution;
	prd.rayOrigin    = rayOrigin;
	prd.rayDir       = rayDirection;
  }
}