
/*
 * Educational Game Engine - Foliage Shader
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
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
Texture2D myTexture : register(t0);
SamplerState linearSampler : register(s0);



//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {

	// Vertex in world coords
	float3				posW			: POSITION;
	// Normal in world coords
	float3				normalW			: NORMAL;
	float4				matDiffuse		: DIFFUSE; // a represents alpha.
	float4				matSpecular		: SPECULAR; // a represents specular power. 
	float2				texCoord		: TEXCOORD;
	float4				posH			: SV_POSITION;
	bool				isFrontFace		: SV_IsFrontFace;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) { 


	FragmentOutputPacket outputFragment;

	float4 baseColour = myTexture.Sample(linearSampler, v.texCoord); 
	// Alpha Test
	// Add Code Here (Clip for alpha < 0.01)
	clip(baseColour.a < 0.01f ? -1 : 1);//Clip for alpha < 0.9

	if (!USE_SHADOW_MAP)//we are rendering depth to the shadow map so dont care about lighting
	{
		outputFragment.fragmentColour = float4(baseColour);
		return outputFragment;
	}
	baseColour.rgb += diffuse.rgb;
	float3 N = normalize(v.normalW);
	if (!v.isFrontFace)
		N = -N;
	
	float percentLit = 1.0f;
	//Lighting
	float3 colour = 0;
	for (int i = 0; i < numLights; i++)
	{
		float pLit = 1.0;
		if (i == 0)
			pLit = min(percentLit + 0.3, 1.0);

		float att = 1.0;
		// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
		float3 lightDir = -lights[i].Vec.xyz; // Directional light
		if (lights[i].Vec.w == 1.0)
		{
			float d = length(v.posW.xyz - lights[i].Vec.xyz);
			att = 1.0 / (d * d * lights[i].Attenuation.z + d * lights[i].Attenuation.y + lights[i].Attenuation.x);
			lightDir = lights[i].Vec.xyz - v.posW; // Positional light
		}

		lightDir = normalize(lightDir);

		//Initialise returned colour to ambient component
		colour += baseColour.xyz * lights[i].Ambient * att;;

		// Add Code Here (Add diffuse light calculation)
		colour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lights[i].Diffuse * pLit * att;

		// Calc specular light
		float specPower = max(specular.a * 1000.0, 1.0f);
		float3 eyeDir = normalize(eyePos - v.posW);
		float3 R = reflect(-lightDir, N);
		// Add Code Here (Specular Factor calculation)	
		float specFactor = pow(max(dot(R, eyeDir), 0.0f), specPower);
		colour += specFactor * specular.xyz * lights[i].Specular * pLit * att;
	}


	//add fog
	float d = length(v.posW.xyz - eyePos.xyz);
	//float3 farColour = colour * 0.2 + float3(0.2, 0.22, 0.2);
	float3 farColour = (0.2 + float3(0.2, 0.22, 0.2)) * lights[0].Diffuse.rgb;
	colour = lerp(colour, farColour, min(d / 40.0 * fog, 1.0));


	outputFragment.fragmentColour = float4(colour, baseColour.a);

	return outputFragment;

}
