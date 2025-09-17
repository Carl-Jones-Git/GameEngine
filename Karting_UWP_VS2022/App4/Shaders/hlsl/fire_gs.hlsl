
// Ensure matrices are row-major
#pragma pack_matrix(row_major)

//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------

struct Light {
	float4				Vec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				Ambient;
	float4				Diffuse;
	float4				Specular;
	float4				Attenuation;// x=constant,y=linear,z=quadratic,w=cutOff
};
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
	Light light[2];
};
cbuffer sceneCBuffer : register(b3) {
	float4						windDir;
	float						Time;
	float						grassHeight;
	float						deltaTime;
	bool				USE_SHADOW_MAP;
	bool				REFLECTION_PASS;
};
//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

struct VertexInputPacket {
	float3 pos : POSITION;   // in object space
	float3 posL : LPOS;   // in object space
	float3 vel :VELOCITY;   // in object space
	float3 data : DATA;   // x=age
};

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct VertexOutputPacket {

	float4 posH  : SV_POSITION;  // in clip space
	float2 texCoord  : TEXCOORD0;
	float4 Z  : DEPTH;
	float alpha : ALPHA;
};

//
// Geometry Shader
//


//Dynamic particle system
//Each vertex is expanded into a screen-aligned quad in view coordinate space.  
[maxvertexcount(8)]
void main(point VertexInputPacket inputVertex[1],
	inout TriangleStream<VertexOutputPacket> outputTriangleStream) {

	//tweakable parameters
	float gPartLife = 1.2;//seconds
	float age = inputVertex[0].data.x;
	float gPartScale = age/3.0+0.03;
	

	bool gDrawReflection = false;

	VertexOutputPacket		outputVertex;
	
	//
	if (age <= 0.0)
	{
		outputVertex.alpha = 0;
		gPartScale = 0;
	}
	else
		outputVertex.alpha = 1.0 - (age / gPartLife);
	//gPartScale = 1;
	// Transform to world space.
	float3 pos = mul(float4(inputVertex[0].pos, 1), worldMatrix);
	//float3 pos = inputVertex[0].pos;

	// Compute world matrix so that billboard faces the camera.
	float3 look = normalize(eyePos - pos);
	float3 right = normalize(cross(float3(0, 1, 0), look));
	float3 up = cross(look, right);
	float2 posL[4];

	posL[0] = float2(1, 1);
	posL[1] = float2(1, -1);
	posL[2] = float2(-1, 1);
	posL[3] = float2(-1, -1);

	float4x4 viewProjMatrix = mul(viewMatrix, projMatrix);
	for (int i = 0; i < 4; i++)
	{
		float3 quadVertexPos = pos + (posL[i].x*right*gPartScale) + (posL[i].y*up*gPartScale);

		// Transform to homogeneous clip space.
		outputVertex.posH = mul(float4(quadVertexPos, 1.0f), viewProjMatrix);
		outputVertex.Z = outputVertex.posH;
		outputVertex.texCoord = float2((posL[i].x + 1)*0.5, (posL[i].y + 1)*0.5);
		outputTriangleStream.Append(outputVertex);
	}

	if (!gDrawReflection) return;
	outputTriangleStream.RestartStrip();

	// Add reflection inbn ground plane
	outputVertex.alpha = outputVertex.alpha / 4;

	// Transform to world space upside down.
	pos = mul(float4(inputVertex[0].pos*float3(1,-1,1), 1), worldMatrix);

	// Compute world matrix so that billboard faces the camera.
	look = normalize(eyePos - pos);
	right = normalize(cross(float3(0, 1, 0), look));
	up = cross(look, right);

	for (int i = 0; i < 4; i++)
	{
		float3 quadVertexPos = pos + (posL[i].x*right*gPartScale) + (posL[i].y*up*gPartScale);

		// Transform to homogeneous clip space.
		outputVertex.posH = mul(float4(quadVertexPos, 1.0f), viewProjMatrix);
		outputVertex.Z = outputVertex.posH;
		outputVertex.texCoord = float2((posL[i].x + 1)*0.5, (posL[i].y + 1)*0.5);
		outputTriangleStream.Append(outputVertex);
	}

}