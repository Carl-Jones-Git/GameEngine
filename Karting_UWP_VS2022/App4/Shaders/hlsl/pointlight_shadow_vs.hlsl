
// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

cbuffer modelCBuffer : register(b4) {

	float4x4			worldViewProjMatrix;
	float4x4			worldITMatrix; // Correctly transform normals to world space
	float4x4			worldMatrix;
};
cbuffer cameraCBuffer : register(b1) {
	float4x4			viewMatrix;
	float4x4			projMatrix;
	float4				eyePos;
};
cbuffer lightCBuffer : register(b2) {
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
};
cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Timer;
	float				grassHeight;
};
cbuffer shadowCBuffer : register(b5) {
	float4x4			shadowTransformMatrix;
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
	float4				posSdw		: SHADOW;
	float4				posH		: SV_POSITION;
};


//-----------------------------------------------------------------
// Vertex Shader
//-----------------------------------------------------------------
VertexOutputPacket main(VertexInputPacket inputVertex) {
	


	VertexOutputPacket outputVertex;
	float4 pos = float4(inputVertex.pos, 1.0);
	float4 posW = mul(pos, worldMatrix);
	// Vertex position in Light coordinates
	outputVertex.posSdw = mul(posW, shadowTransformMatrix);
	// Lighting is calculated in world space.
	outputVertex.posW = posW.xyz;
	// Transform normals to world space with gWorldIT.
	outputVertex.normalW = mul(float4(inputVertex.normal, 1.0f), worldITMatrix).xyz;
	// Pass through material properties
	outputVertex.matDiffuse = inputVertex.matDiffuse;
	outputVertex.matSpecular = inputVertex.matSpecular;
	// .. and texture coordinates.
	outputVertex.texCoord = inputVertex.texCoord;
	// Finally transform/project pos to screen/clip space posH
	outputVertex.posH = mul(pos, worldViewProjMatrix);




	return outputVertex;
}
