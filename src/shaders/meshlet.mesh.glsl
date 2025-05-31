#version 450

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_NV_mesh_shader : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 126) out;

struct Vertex 
{
	float16_t vx, vy, vz, vw;
	uint8_t nx, ny, nz, nw;
	float16_t tu, tv;
};

layout(binding = 0)readonly buffer Vertices
{
	Vertex vertices[];
};

struct Meshlet {
	uint vertices[64];
	uint8_t indices[126*3];
	uint8_t triangleCount;
	uint8_t vertexCount;
};

layout(binding = 1)readonly buffer Meshlets
{
	Meshlet meshlets[];
};

layout(location = 0)out vec4 color[];

void main() {
	uint mi = gl_WorkGroupID.x;
	uint ti = gl_LocalInvocationID.x;

	uint vertexCount = uint(meshlets[mi].vertexCount);
	uint triangleCount = uint(meshlets[mi].triangleCount);
	uint indexCount = triangleCount * 3;

	for (uint i = ti; i < vertexCount; i += 32) {
		uint vi = meshlets[mi].vertices[i];
		Vertex v = vertices[vi];

		vec3 position = vec3(v.vx, v.vy, v.vz);
		vec3 normal = vec3(v.nx, v.ny, v.nz) / 127.0 - 1.0;
		vec2 texcoord = vec2(v.tu, v.tv);

		gl_MeshVerticesNV[i].gl_Position = vec4(position + vec3(0.0, 0.0, 0.5), 1.0);

		float meshletColorR = float(mi % 255u);
		float meshletColorG = float(mi % 127u);
		float meshletColorB = float(mi % 63u);
		//color[i] = vec4(normal * 0.5 + vec3(0.5), 1.0);
		color[i] = vec4(meshletColorR, meshletColorG, meshletColorB, 1.0);
	}

	for (uint i = ti; i < indexCount; i += 32) {
		gl_PrimitiveIndicesNV[i] = uint(meshlets[mi].indices[i]);
	}

	if (ti == 0) {
		gl_PrimitiveCountNV = uint(meshlets[mi].triangleCount);
	}
}