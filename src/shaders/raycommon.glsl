#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct hitPayload
{
  vec3 hitValue;
  uint seed;
};

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

struct Material {
    vec4 Diffuse;
    uint DiffuseTextureId;
    float Fuzziness;
    float RefractionIndex;
	float transparency;
	uint matIndex;
	vec3 _padding;
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
	};