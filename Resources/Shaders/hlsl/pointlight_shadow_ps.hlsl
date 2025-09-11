
//
// Model a simple light
//
#define NO_SHADOW_MAP 1
#define ENV_MAP_PASS 1 << 1
#define SHADOW_MAP_PASS 1<<2
#define WITH_SHADOW_MAP 1<<3
// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

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
	bool				USE_SHADOW_MAP;
};


//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D myTexture : register(t0);
Texture2D shadowMap : register(t1);
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
	float3 N = normalize(v.normalW);
	float4 baseColour = v.matDiffuse;
	baseColour = baseColour * myTexture.Sample(linearSampler, v.texCoord);

	//Initialise colour to ambient component
	float3 colour = baseColour.xyz* lightAmbient;

	//Test if this pixel is in shadow
	if (USE_SHADOW_MAP)
	{
		v.posSdw.xyz /= v.posSdw.w;// Complete projection by doing division by w.
		// Check if pixel is outside of the bounds of the shadowmap
		if (v.posSdw.x < 1 && v.posSdw.x>0 && v.posSdw.y < 1 && v.posSdw.y>0)
		{
			float depthShadowMap = shadowMap.Sample(linearSampler, v.posSdw.xy).r;
			// Test if pixel depth is >= the value in the shadowmap(i.e behind/obscured) 
			if (v.posSdw.z >= (depthShadowMap + 0.00001)){//add small bias to avoid z fighting
				// If this pixel is behind the value in the shadowmap it is in shadow
				// so return ambient only. If not complete full lighting calculation 
				outputFragment.fragmentColour = float4(colour, baseColour.a);
				return outputFragment;
			}
		}
	}

	// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	float3 lightDir = -lightVec.xyz; // Directional light
	if (lightVec.w == 1.0) lightDir = lightVec.xyz - v.posW; // Positional light
	lightDir = normalize(lightDir);

	// Add diffuse light if relevant (otherwise we end up just returning the ambient light colour)
	colour += max(dot(lightDir, N), 0.0f) *baseColour.xyz * lightDiffuse;

	// Calculate specular light
	float specPower = max(v.matSpecular.a*1000.0, 1.0f);
	float3 eyeDir = normalize(eyePos - v.posW);
	float3 R = reflect(-lightDir, N);
	float specFactor = pow(max(dot(R, eyeDir), 0.0f), specPower);
	colour += specFactor * v.matSpecular.xyz * lightSpecular;

	outputFragment.fragmentColour = float4(colour, baseColour.a);
	return outputFragment;
}
