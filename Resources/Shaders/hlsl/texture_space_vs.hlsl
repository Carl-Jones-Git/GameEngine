




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
	//float2				texCoord1	: TEXCOORD1;
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

	// Lighting is calculated in world space.
	//Add Code Here(Transform vertex position to world coordinates)
	outputVertex.posW = mul(float4(inputVertex.pos, 1.0f), worldMatrix);
	// Transform normals to world space with gWorldIT.
	outputVertex.normalW = mul(float4(inputVertex.normal, 1.0f), worldITMatrix).xyz;
	// Pass through material properties
	outputVertex.matDiffuse = inputVertex.matDiffuse;
	outputVertex.matSpecular = inputVertex.matSpecular;
	// .. and texture coordinates.
	outputVertex.texCoord = inputVertex.texCoord;

	// Finally transform/project pos to screen/clip space posH
	outputVertex.posH = float4(inputVertex.pos, 1.0f);
	outputVertex.posH.x = inputVertex.texCoord.x * 2.0 - 1.0;
	outputVertex.posH.y = inputVertex.texCoord.y * 2.0 + 1.0;
	outputVertex.posH.z = 0.5;

	return outputVertex;
}
