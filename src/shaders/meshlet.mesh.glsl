#version 450

#extension GL_EXT_shader_16bit_storage : require
#extension GL_EXT_shader_8bit_storage : require
#extension GL_NV_mesh_shader : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

#extension GL_GOOGLE_include_directive : require

#include "mesh.h"

#define DEBUG 1

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(triangles, max_vertices = 64, max_primitives = 126) out;

layout(binding = 0)readonly buffer Vertices
{
	Vertex vertices[];
};

layout(binding = 1)readonly buffer Meshlets
{
	Meshlet meshlets[];
};

in taskNV block{
	uint meshletIndices[32];
};

layout(location = 0)out vec4 color[];

uint hash(uint a)
{
	a = (a + 0x7ed55d16) + (a << 12);
	a = (a ^ 0xc761c23c) ^ (a >> 19);
	a = (a + 0x165667b1) + (a << 5);
	a = (a + 0xd3a2646c) ^ (a << 9);
	a = (a + 0xfd7046c5) + (a << 3);
	a = (a ^ 0xb55a4f09) ^ (a >> 16);
	return a;
}

void main() {
	uint mi = meshletIndices[gl_WorkGroupID.x];
	uint ti = gl_LocalInvocationID.x;

	uint vertexCount = uint(meshlets[mi].vertexCount);
	uint triangleCount = uint(meshlets[mi].triangleCount);
	uint indexCount = triangleCount * 3;

#if DEBUG
	uint mhash = hash(mi);
	vec3 hashColor = vec3(float(mhash & 255), float((mhash >> 8) & 255), float((mhash >> 16) & 255)) / 255.0;
#endif

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
		color[i] = vec4(normal * 0.5 + vec3(0.5), 1.0);
#if DEBUG
		color[i] = vec4(hashColor, 1.0);
#endif
	}

	for (uint i = ti; i < indexCount; i += 32) {
		gl_PrimitiveIndicesNV[i] = uint(meshlets[mi].indices[i]);
	}

	if (ti == 0) {
		gl_PrimitiveCountNV = uint(meshlets[mi].triangleCount);
	}
}