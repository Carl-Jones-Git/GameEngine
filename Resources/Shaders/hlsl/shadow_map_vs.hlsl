/*
 * Educational Game Engine - Shadow Map Shader
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */
// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer Camera : register(b0) {

	float4x4			viewProjMatrix;
	float3				eyePos;
};

// Per-object world transform data
cbuffer WorldTransform : register(b1) {

	float4x4			worldMatrix;
	float4x4			normalMatrix; // Inverse transpose of worldMatrix
};


//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------
struct VertexInputPacket {

	float3				pos			: POSITION;
	float3				normal		: NORMAL;
	float4				matDiffuse	: DIFFUSE;
	float4				matSpecular	: SPECULAR;
	float2				texCoord	: TEXCOORD;
};


struct VertexOutputPacket {

	// Vertex in world coords
	float3				posW		: POSITION;

	// Normal in world coords
	float3				normalW		: NORMAL;
	
	
	float4				matDiffuse	: DIFFUSE;
	float4				matSpecular	: SPECULAR;
	float2				texCoord	: TEXCOORD;
	float4				posH		: SV_POSITION;
};


//-----------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------
VertexOutputPacket main(VertexInputPacket inputVertex) {
	
	VertexOutputPacket outputVertex;

	float4 pos = float4(inputVertex.pos, 1.0);

	float4x4 wvpMatrix = mul(worldMatrix, viewProjMatrix);

	outputVertex.posH = mul(pos, wvpMatrix);
	outputVertex.posW = mul(pos, worldMatrix).xyz;

	// multiply the input normal by the inverse-transpose of the world transform matrix
	outputVertex.normalW = mul(float4(inputVertex.normal, 1.0), normalMatrix).xyz;

	outputVertex.matDiffuse = inputVertex.matDiffuse;
	outputVertex.matSpecular = inputVertex.matSpecular;

	outputVertex.texCoord = inputVertex.texCoord;

	return outputVertex;
}
