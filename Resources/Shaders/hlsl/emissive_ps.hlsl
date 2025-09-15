/*
 * Educational Game Engine - Simple Emissive Pixel Shader
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */

// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Time;
	float				grassHeight;
	int					USE_SHADOW_MAP;
	int					QUALITY;
	float				fog;
};

cbuffer materialCBuffer : register(b4) {

	float4 emissive;
	float4 ambient;
	float4 diffuse;
	float4 specular;
	int useMaterial;
};

//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D emissiveTexture : register(t0);
SamplerState linearSampler : register(s0);

static const int DIFFUSE_MAP = 1 << 0;
static const int EMISSIVE_MAP = 1 << 5;

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


//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) {

	FragmentOutputPacket outputFragment;

	float4 emissiveColour = float4(1,1,1,1);
	//if (useMaterial & (DIFFUSE_MAP | EMISSIVE_MAP)) //sample emissive map
	//	emissiveColour = emissiveTexture.Sample(linearSampler, v.texCoord );
	//outputFragment.fragmentColour = float4(emissiveColour.xyz , (emissiveColour.r + emissiveColour.g + emissiveColour.b)/3);
	outputFragment.fragmentColour = emissiveColour;
	return outputFragment;

}
