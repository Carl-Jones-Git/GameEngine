
/*
 * Educational Game Engine - Grass effect - Modified Shell Shader
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */
// Ensure matrices are row-major
#pragma pack_matrix(row_major)

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
};
cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Time;
	float				grassHeight;
	int					USE_SHADOW_MAP;
	int					QUALITY;
	float				fog;
};

cbuffer shadowCBuffer : register(b5) {
	float4x4			shadowTransformMatrix;
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

Texture2D noiseMap: register(t8);

//-----------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------
vertexOutputPacket main(vertexInputPacket inputVertex) {
	float grassScaleFactor = 50.0;

	// Finally transform/project pos to screen/clip space posH
	float3 pos = inputVertex.pos;
	pos.y += grassHeight * grassScaleFactor;
	float k = pow(grassHeight * 1800.0f, 2.0f);

	//Old code
	//float3 gWindDir = float3((sin(Time+pos.x+pos.y)+1.0f )* 0.03, 0, sin(Time*4 + pos.x + pos.y)*0.01);
	//pos +=gWindDir * k
	
	//New code
	int tileScale=500, speedScale=10;
	int width, Height;
	noiseMap.GetDimensions( width, Height);
	pos.z +=k*(noiseMap.Load(int4((inputVertex.texCoord.x* tileScale +Time* speedScale)% width, (inputVertex.texCoord.y * tileScale + Time * speedScale) % Height, 0,0), 0).r - 0.5 )/4.0f;
	pos.x +=k*(noiseMap.Load(int4((inputVertex.texCoord.x* tileScale + Time * speedScale*2) % width, (inputVertex.texCoord.y * tileScale + Time * speedScale*2) % Height, 0, 0), 0).r-0.5)/4.0f ;

	vertexOutputPacket outputVertex;
	float4x4 WVP = mul(worldMatrix, mul(viewMatrix, projMatrix));
	// Lighting is calculated in world space.
	outputVertex.posW = mul(float4(pos, 1.0f), worldMatrix).xyz;
	//float4 posW = mul(pos, worldMatrix);
	outputVertex.posSdw = mul(float4(outputVertex.posW.x, outputVertex.posW.y, outputVertex.posW.z, 1.0f), shadowTransformMatrix);
	// Transform normals to world space with gWorldIT.,
	outputVertex.normalW = mul(float4(inputVertex.normal, 0.0f), worldITMatrix).xyz;
	outputVertex.tangentW = mul(float4(inputVertex.tangent, 0.0f), worldMatrix).xyz;
	// Pass through material properties
	outputVertex.matDiffuse = inputVertex.matDiffuse;
	outputVertex.matSpecular = inputVertex.matSpecular;
	// .. and texture coordinates.
	outputVertex.texCoord = inputVertex.texCoord;
	outputVertex.posH = mul(float4(pos, 1.0), WVP);

	return outputVertex;
}
