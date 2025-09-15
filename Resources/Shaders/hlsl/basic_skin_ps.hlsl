
//
// Skin Pixel Shader modified by Carl Jones 2023 from:
//https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-14-advanced-techniques-realistic-real-time-skin
//

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


float PHBeckmann(float ndoth, float m)
{
	float alpha = acos(ndoth);
	float ta = tan(alpha);
	float val = 1.0 / (m * m * pow(ndoth, 4.0)) * exp(-(ta * ta) / (m * m));
	return val;
}

float fresnelReflectance(float3 H, float3 V, float F0)
{
	float base = 1.0 - dot(V, H);
	float exponential = pow(base, 5.0);
	float fres = exponential + F0 * (1.0 - exponential);
	return fres;
}

float KS_Skin_Specular(
	float3 N, // Bumped surface normal  
	float3 L, // Points to light  
	float3 V, // Points to eye  
	float m,  // Roughness  
	float rho_s // Specular brightness  
)
{
	float result = 0.0;
	float ndotl = dot(N, L);
	if (ndotl > 0.0)
	{
		float3 h = L + V; // Unnormalized half-way vector  
		float3 H = normalize(h);
		float ndoth = dot(N, H);
		float PH = PHBeckmann(ndoth, m);
		float F = fresnelReflectance(H, V, 0.028);
		float frSpec = max(PH * F / dot(h, h), 0);
		result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s  
	}
	return result;
}


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

	float3 eyeDir = normalize(eyePos - v.posW);
	float m = 0.17;  // Roughness  
	float rho_s = 0.42;// Specular brightness
	float3 colour = 0;
	for (int i = 0; i < numLights; i++)
	{
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
		colour += lights[i].Ambient.xyz * baseColour.xyz *att;

		// Add diffuse light
		colour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lights[i].Diffuse * att;

		// Calc specular light
		float specularValue = KS_Skin_Specular(N, lightDir, eyeDir, m, rho_s);
		colour += specularValue * v.matSpecular * lights[i].Specular.xyz * att;
	}
	outputFragment.fragmentColour = float4(colour, baseColour.a);
	return outputFragment;
}
