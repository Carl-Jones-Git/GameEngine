

// Ensure matrices are row-major
#pragma pack_matrix(row_major)

cbuffer cameraCBuffer : register(b1) {
	float4x4			viewMatrix;
	float4x4			projMatrix;
	float4				eyePos;
};
cbuffer materialCBuffer : register(b4) {

	float4 emissive;
	float4 ambient;
	float4 diffuse;
	float4 specular;
	int useMaterial;
};
//----------------------------
// Input / Output structures
//----------------------------
struct vertexInputPacket {

	float3				pos			: POSITION;
	float3				posL		: LPOS;
	float4				colour		: COLOR;
};


struct vertexOutputPacket {

	float4				colour		: COLOR;
	float2				texCoord	:TEXCOORD;
	float4				posH		: SV_POSITION;
};

Texture2DMS  <float>depth: register(t5);
//
// Vertex shader
//
vertexOutputPacket main(vertexInputPacket inputVertex) {

	vertexOutputPacket outputVertex;

	float size = 0.5*diffuse.r;
	if(diffuse.r+ diffuse.g+ diffuse.b>2.9f)
	size = 1.5;
	float4x4 viewProjMatrix = mul(viewMatrix, projMatrix);
	float4 pos = mul(float4(inputVertex.pos, 1.0f), viewProjMatrix);
	//1280, 720
	int clipPosX = ((pos.x / pos.w)*0.5 + 0.5)* 1280;
	int clipPosY = ((pos.y / -pos.w)*0.5 + 0.5)* 720;

	float d = 0.0;
	
	for (int i = -5; i <= 5;i++)
		for (int j = -5; j <= 5; j++)
			d=max(d,depth.Load(int4(clipPosX+i, clipPosY+j, 0, 0), 0).r);

	if (d >0.999)
	{
		pos.xy = lerp(pos.xy, -pos.xy,diffuse.a);
		pos.x += inputVertex.posL.x*pos.w*size;
		pos.y += inputVertex.posL.y*pos.w*size;
		pos.z = 0;
	}
	else
		pos = float4(0, 0, 0, 0);
	// Transform to homogeneous clip space.
	outputVertex.colour = diffuse;
	outputVertex.posH = pos;// 
	outputVertex.texCoord = float2((inputVertex.posL.x + 1)*0.5, (inputVertex.posL.y + 1)*0.5);

	return outputVertex;
}
