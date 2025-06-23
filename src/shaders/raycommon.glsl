#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct hitPayload
{
  vec3 hitValue;
  uint seed;
  int depth;
  vec3 attenuation;
  int  done;
  vec3 rayOrigin;
  vec3 rayDir;
  vec3 weight;
};

struct EmissiveTriangle
{
  vec4 x0;
  vec4 x1;
  vec4 x2;
  vec4 normal;
  vec4 emission;
  float area;
  float importance;  // area × luminance of emission
  float extra[2]; // 16 bytes align
};

struct Sample
{
    vec3 origin;
    vec3 hit;
    EmissiveTriangle light;
    vec3 lightPos;
    vec3 bounceDir;
    float weight;
};

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

struct Material {
    vec4 colorFactors;
    vec4 metalRoughFactors;
	vec3 emissiveFactors;
	float emissivePower;
    float transparency;
	float extra[51];
};

struct shadowPayload
{
  bool isHit;
  uint seed;
};
struct GPUInstanceBuffers {
    uint64_t vertexBufferAddress;
	uint64_t indexBufferAddress;
	uint64_t materialBufferAddress;
	uint64_t materialIndicesBufferAddress;
	uint64_t emissiveTrianglesBufferAddress;
};
