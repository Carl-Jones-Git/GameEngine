/*
 * Educational Game Engine - Skinning Vertex Shader
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */

// Ensure matrices are row-major
#pragma pack_matrix(row_major)
#define MAX_BONES 120
//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer modelCBuffer : register(b0) {
	float4x4			worldMatrix;
	float4x4			worldITMatrix; // Correctly transform normals to world space
};
cbuffer cameraCbuffer : register(b1) {
	float4x4			viewMatrix;
	float4x4			projMatrix;
	float4				eyePos;
}
cbuffer lightCBuffer : register(b2) {
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
	float4				lightAttenuation;
};
cbuffer shadowCBuffer : register(b5) {
	float4x4			shadowTransformMatrix;
};
cbuffer boneCBuffer : register(b6) {
	float4x4 bones[MAX_BONES];
};
//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------
struct vertexInputPacket {

	float3				pos			: POSITION;
	float3				normal		: NORMAL;
	float3				tangent		: TANGENT;
	float4				matDiffuse	: DIFFUSE; // a represents alpha.
	float4				matSpecular	: SPECULAR;  // a represents specular power. 
	float2				texCoord	: TEXCOORD;
	float4				weights0	: WEIGHTS0;
	float4				weights1	: WEIGHTS1;
	float4				weights2	: WEIGHTS2;
	int4				indices0	: INDICES0;
	int4				indices1	: INDICES1;
	int4				indices2	: INDICES2;
};


struct vertexOutputPacket {


	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float3				tangentW		: TANGENT;
	float4				matDiffuse		: DIFFUSE;
	float4				matSpecular		: SPECULAR;
	float2				texCoord		: TEXCOORD;
	float4				posSdw			: SHADOW;
	float4				posH			: SV_POSITION;
};


//-----------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------
vertexOutputPacket main(vertexInputPacket inputVertex) {

	float4x4 WVP = mul(worldMatrix, mul(viewMatrix,projMatrix));
	
	vertexOutputPacket outputVertex;
	
	
	float3 posL = 0;
	float3 normL = 0;
	float3 tangL = 0;
	float weights[12] = { inputVertex.weights0 ,inputVertex.weights1, inputVertex.weights2 };
	float indices[12] = { inputVertex.indices0 ,inputVertex.indices1, inputVertex.indices2 };

		for (int i = 0; i < 12; i++)
		{
			// Assume no nonuniform scaling when transforming normals, so
			// that we do not have to use the inverse-transpose.
			if (indices[i] >= 0)
			{
				posL += weights[i] * mul(float4(inputVertex.pos, 1.0f), bones[indices[i]]).xyz;
				normL += weights[i] * mul(inputVertex.normal, ((float3x3)bones[indices[i]])).xyz;
				tangL += weights[i] * mul(inputVertex.tangent, (float3x3)bones[indices[i]]);
			}
		}


	// Lighting is calculated in world space.
	//Add Code Here(Transform vertex position to world coordinates)
	outputVertex.posW = mul(float4(posL, 1.0f), worldMatrix);
	outputVertex.posSdw = mul(float4(outputVertex.posW, 1.0f), shadowTransformMatrix);
	// Transform normals to world space with WorldIT.
	outputVertex.normalW = mul(float4(normL, 0.0f), worldITMatrix).xyz;
	outputVertex.tangentW = mul(float4(tangL,0.0f), worldMatrix).xyz;
	// Pass through material properties
	outputVertex.matDiffuse = inputVertex.matDiffuse;
	outputVertex.matSpecular = inputVertex.matSpecular;
	// .. and texture coordinates.
	outputVertex.texCoord = inputVertex.texCoord;
	// Finally transform/project pos to screen/clip space posH
	outputVertex.posH = mul(float4(posL, 1.0f), WVP);

	return outputVertex;
}
