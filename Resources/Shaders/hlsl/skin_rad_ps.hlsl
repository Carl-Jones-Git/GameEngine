/*
 * Educational Game Engine - Skin Radiance Shader
 * modified by Carl Jones 2023 from:
 * https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-14-advanced-techniques-realistic-real-time-skin
 *
 */

// Ensure matrices are row-major
#pragma pack_matrix(row_major)
#define MAX_LIGHTS 10

//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------
struct Light
{
	float4				Vec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				Ambient;
	float4				Diffuse;
	float4				Specular;
	float4				Attenuation;
	float4				Cone;
};
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
	Light lights[MAX_LIGHTS];
};

cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Time;
	float				grassHeight;
	int					USE_SHADOW_MAP;
	int					QUALITY;
	float				fog;
	int					numLights;
};


//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------


// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {

	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float3				tangentW		: TANGENT;
	float4				matDiffuse		: DIFFUSE; // a represents alpha.
	float4				matSpecular		: SPECULAR; // a represents specular power. 
	float2				texCoord		: TEXCOORD;
	float4				posSdw			: SHADOW;
	float4				posH			: SV_POSITION;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};




//
// Textures
//

// Assumes texture bound to t0, t1 t2 and sampler bound to sampler s0
Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specMap : register(t2);
SamplerState linearSampler : register(s0);

//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) { 

	FragmentOutputPacket outputFragment;


	// Calc normals using object space normal map (u and v need to be flipped)
	float3 N = normalize(v.normalW);
	float3 Nt = (normalMap.Sample(linearSampler, v.texCoord).xyz * 2.0) - 1.0;
	Nt = mul(float4(Nt, 1.0), worldITMatrix);
	Nt = normalize(Nt);
	Nt.y = -Nt.y;
	Nt.x = -Nt.x;
	N = normalize(N + Nt * 0.6);

	float4 baseColour = v.matDiffuse;
	baseColour *= diffuseMap.Sample(linearSampler, v.texCoord) * 1.2;

	float3 colour = 0;
	for (int i = 0; i < numLights; i++)
		//for (int i = 1; i < 2; i++)
	{
		float att = 1.0;
		// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
		float3 lightDir = -lights[i].Vec.xyz; // Directional light
		if (lights[i].Vec.w == 1.0)
		{
			float d = length(v.posW.xyz - lights[i].Vec.xyz);
			att = 1.0 / (d * d * lights[i].Attenuation.z + d * lights[i].Attenuation.y + lights[i].Attenuation.x);
			//if (i == 1)
			//	att = (1.0 / (d * d * 0.1)) * (((lights[1].Vec.z) / 4.0) * 0.9 + 0.03) * 6.5;
			lightDir = lights[i].Vec.xyz - v.posW; // Positional light

		}


		lightDir = normalize(lightDir);

		//Initialise returned colour to ambient component
		colour += lights[i].Ambient.xyz * baseColour.xyz * att;;

		// Add diffuse light
		colour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lights[i].Diffuse * att;
	}

	outputFragment.fragmentColour = float4(colour, baseColour.a);
	return outputFragment;

}
