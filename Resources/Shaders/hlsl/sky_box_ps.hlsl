
//
// Model a simple light
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)

cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Time;
	float				grassHeight;
	int				USE_SHADOW_MAP;
	float	fog;
};
//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------


//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
// Assumes texture bound to texture t0 and sampler bound to sampler s0

TextureCube cubeMapNight : register(t0);
TextureCube cubeMapDay : register(t6);
SamplerState linearSampler : register(s0);


//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {


	float3				texCoord		: TEXCOORD;
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



	// float3 colour= cubeMapDay.Sample(linearSampler, v.texCoord).xyz;


	float DNCycle = sin(Time / 20.0f);// / 2 + 0.5f;
	 float3 lightFactor = saturate(float3(0.5 + (DNCycle * 0.4), 0.5 + (DNCycle * 0.4), 0.6 + (DNCycle * 0.3))+float3(0.0,0.0,0.3));
	// float3 colour = float4(cubeMapNight.Sample(linearSampler, v.texCoord).xyz * (1 - DNCycle) * lightFactor, 1) + float4(cubeMapDay.Sample(linearSampler, v.texCoord).xyz * (DNCycle)*lightFactor, 1);
	 float3 colour = float4(cubeMapDay.Sample(linearSampler, v.texCoord).xyz * float3(0.1, 0.1, 0.7) * (1 - DNCycle) * lightFactor, 1)+float4(cubeMapDay.Sample(linearSampler, v.texCoord).xyz * (DNCycle)*lightFactor, 1);



	//add fog
	 float d = 100.0f;// length(v.posW.xyz - eyePos.xyz);
	//float3 farColour = colour * 0.2 + float3(0.2, 0.22, 0.2);
	float3 farColour = 0.2 + float3(0.2, 0.22, 0.2);//colour*
	colour = lerp(colour, farColour, min(d / 40.0 * fog, 1.0));
	outputFragment.fragmentColour = float4(colour, 1);
	return outputFragment;

}
