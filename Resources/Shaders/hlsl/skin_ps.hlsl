
//
// Skin shader
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




//
// Textures
//
// Assumes texture bound to t0 - t7 and sampler bound to sampler s0
// Blurred irradiance textures
Texture2D irrad1Tex : register(t0);
Texture2D irrad2Tex : register(t1);
Texture2D irrad3Tex : register(t2);
Texture2D irrad4Tex : register(t3);
Texture2D irrad5Tex : register(t4);
Texture2D irrad6Tex : register(t5);
Texture2D normalMap : register(t6);
Texture2D diffuseMap : register(t7);//diffuse

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
// Pixel Shader - Final integration  pass
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket IN) {

	
	FragmentOutputPacket outputFragment;

	// Calc normals using object space normal map (u and v need to be flipped)
	float3 N = normalize(IN.normalW);
	float3 Nt = (normalMap.Sample(linearSampler, IN.texCoord).xyz * 2.0) - 1.0;
	Nt = mul(float4(Nt, 1.0), worldITMatrix);
	Nt = normalize(Nt);
	Nt.y = -Nt.y;
	Nt.x = -Nt.x;
	N = normalize(N + Nt * 0.6);

		// Calculate attenuation
	float d = length(lights[1].Vec - IN.posW);
	//float att = 1.0 / (d * d * 0.2 + d * 0.5 + 1.0f);
	float af = 0.1;
	float att = min(1.0 / (d * d * af), 3.0f);
	//float att = 1.0 / (d * d * 0.5);
	//att = (1.0 / (d * d * af)) * (((lightVec.z - 0.0) / 8.0) * 0.9 + 0.03) * 6.5;
	// 
	float boost = clamp((8 - d)*0.5, 1, 5);
	boost=(1.0 + max(min((5 - 1.0) * min(att / d, 1.0), 3.5), 0.0));
	boost = (1.0 + max(min(((5 - 1.0) / 20.0) * min(att / d, 1.0), 2.5), 0.0));
	//// RGB Gaussian weights that define skin profiles
	float3 gauss1w = float3(0.233, 0.455, 0.649);
	float3 gauss2w = float3(0.100, 0.336, 0.344);
	float3 gauss3w = float3(0.118, 0.198, 0.000);
	float3 gauss4w = float3(0.113, 0.007, 0.007);// *(1.0 + max(min((5 - 1.0) * min(att / d, 1.0), 3.5), 0.0));// *lightAmbient.w;// *(1.0 + max(min(((lightAmbient.w - 1.0) / 10.0) * min(att / d, 1.0), 2.5), 0.0));
	float3 gauss5w = float3(0.358, 0.004, 0.000) * (1.0 + max(min(((lights[1].Ambient.w - 1.0) / 20.0) * min(att / d, 1.0), 2.5), 0.0));
	float3 gauss6w = float3(0.078, 0.000, 0.000) * (1.0 + max(min((lights[1].Ambient.w - 1.0) * min(att / d, 1.0), 3.5), 0.0));// *lightAmbient.w;

	
	//float3 gauss5w = float3(0.358, 0.004, 0.000) * boost;// *(1.0 + max(min(((5 - 1.0) / 20.0) * min(att / d, 1.0), 2.5), 0.0));
	//float3 gauss6w = float3(0.078, 0.000, 0.000) * (1.0 + max(min((lights[1].Ambient.w - 1.0) * min(att / d, 1.0), 3.5), 0.0));
	
	//Alternative skin profiles
	//// RGB Gaussian weights that define skin profiles
	//float3 gauss1w = float3( 0.455, 0.233, 0.649);
	//float3 gauss2w = float3( 0.336, 0.100, 0.344);
	//float3 gauss3w = float3( 0.198, 0.118, 0.000);
	//float3 gauss4w = float3( 0.007, 0.113, 0.007);
	//float3 gauss5w = float3( 0.004, 0.358, 0.000);
	//float3 gauss6w = float3(0.000, 0.078, 0.000);
	//// RGB Gaussian weights that define skin profiles
	//float3 gauss1w = float3(0.22, 0.437, 0.635);
	//float3 gauss2w = float3( 0.101, 0.355, 0.365);
	//float3 gauss3w = float3(0.119, 0.208, 0);
	//float3 gauss4w = float3(0.114, 0, 0);
	//float3 gauss5w = float3(0.364, 0, 0) * (1.0 + max(min((lightAmbient.w - 1.0) * min(att / d, 1.0), 2.5), 0.0));
	//float3 gauss6w = float3(0.080, 0, 0);// *(1.0 + max(min((lightAmbient.w - 1.0) * min(att / d, 1.0), 2.5), 0.0));// *lightAmbient.w;



	// The total diffuse light exiting the surface
	float4 baseColour = IN.matDiffuse;
		baseColour = baseColour * diffuseMap.Sample(linearSampler, IN.texCoord)*1.2;
	//float mix = 0.8;
	//float3 diffuseLight = pow(baseColour, mix);
	float3 diffuseLight = 0;

	float3 irrad1tap = irrad1Tex.Sample(linearSampler, IN.texCoord);
	float3 irrad2tap = irrad2Tex.Sample(linearSampler, IN.texCoord);
	float3 irrad3tap = irrad3Tex.Sample(linearSampler, IN.texCoord);
	float3 irrad4tap = irrad4Tex.Sample(linearSampler, IN.texCoord);
	float3 irrad5tap = irrad5Tex.Sample(linearSampler, IN.texCoord);
	float3 irrad6tap = irrad6Tex.Sample(linearSampler, IN.texCoord);
	diffuseLight += gauss1w * irrad1tap.xyz;
	diffuseLight += gauss2w * irrad2tap.xyz;
	diffuseLight += gauss3w * irrad3tap.xyz;
	diffuseLight += gauss4w * irrad4tap.xyz;
	diffuseLight += gauss5w * irrad5tap.xyz;
	diffuseLight += gauss6w * irrad6tap.xyz;
	// Renormalize diffusion profiles to white
	float3 normConst = gauss1w + gauss2w + gauss3w + gauss4w + gauss5w + gauss6w;
	diffuseLight /= normConst; // Renormalize to white diffuse light
	float3 colour = diffuseLight;

	float3 eyeDir = normalize(eyePos - IN.posW);
	float m = 0.17;  // Roughness  
	float rho_s = 0.42;// Specular brightness
	for (int i = 0; i < numLights; i++)
	{
		float att = 1.0;
		// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
		float3 lightDir = -lights[i].Vec.xyz; // Directional light
		if (lights[i].Vec.w == 1.0)
		{
			float d = length(IN.posW.xyz - lights[i].Vec.xyz);
			att = 1.0 / (d * d * lights[i].Attenuation.z + d * lights[i].Attenuation.y + lights[i].Attenuation.x);
			lightDir = lights[i].Vec.xyz - IN.posW; // Positional light
			//if (i == 1)
			//	att = (1.0 / (d * d * 0.1)) * (((lights[1].Vec.z) / 4.0) * 0.9 + 0.03) * 6.5;
		}
		lightDir = normalize(lightDir);

		// Calc specular light
		float specularValue = KS_Skin_Specular(N, lightDir, eyeDir, m, rho_s);
		colour += specularValue * IN.matSpecular * lights[i].Specular.xyz * att;
	}
	outputFragment.fragmentColour = float4(colour, baseColour.a);

	return outputFragment;
}
