
//
// Model a simple light
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

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
	float4				lightVec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				lightAmbient;
	float4				lightDiffuse;
	float4				lightSpecular;
	float4				lightAttenuation;
};

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
Texture2D diffMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D specMap : register(t2);
SamplerState linearSampler : register(s0);

TextureCube envMap : register(t6);
Texture2D shadowMap: register(t7);
SamplerComparisonState SamShadow : register(s7);

///////// PARAMETERS Could be added to CBUFFER //////////////////
static const int USE_MATERIAL_ALL = 0;
static const int DIFFUSE_MAP = 1 << 0;
static const int NORMAL_MAP = 1 << 1;
static const int SPECULAR_MAP = 1 << 2;
static const int OPACITYMAP = 1 << 3;
//...
static const int CHROME = 1 << 8;


static float FresnelBias = 0.0;// 0.1;//0.3;
static float FresnelExp = 1.0;//0.5;//4;

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


float inShadow(FragmentInputPacket v, float zBias = 0.001f)
{
	float percentLit = 1.0f;
	v.posSdw.xyz /= v.posSdw.w;// Complete projection by doing division by w.
	float ImageWidth = 2048;//8192;//4096;// 8192;	
	if (QUALITY == 2)
		ImageWidth = 4096;
	else if (QUALITY > 2)
		ImageWidth = 8192;

	float textureScale = 1.0f / ImageWidth;//get width of a texel in uv coords

	// Check if pixel is outside of the bounds of the shadowmap
	if (v.posSdw.x < 1 && v.posSdw.x>0 && v.posSdw.y < 1 && v.posSdw.y>0)
	{
		float dist = length(v.posW.xyz - eyePos.xyz);
		//float zBias = 0.001f;
		//if pixel is close use 9x pcf filtering else dont filter
		if (dist < 5.0f)
		{
			//16x filter see GPUGems Chapter11 ShadowMap Antialiasing and Percentage closer filtering
			//Actually using 9x not 16x
			float sum = 0; float x, y;
			float count = 0;

			for (y = -0.5f; y <= 0.5f; y += 0.5f)
				for (x = -0.5f; x <= 0.5f; x += 0.5f)
				{
					sum += shadowMap.SampleCmp(SamShadow, v.posSdw.xy + textureScale * (float2(x, y)), v.posSdw.z + zBias);//add small bias to avoid z fighting
					count++;
				}
			percentLit = max(min((sum / count), 1.0f), 0.0f);
		}
		else
			percentLit = shadowMap.SampleCmp(SamShadow, v.posSdw.xy, v.posSdw.z + zBias);//add small bias to avoid z fighting
	}
	return percentLit;
}
//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) {
	clip((v.posW.y < 0.9f && length(v.posW.xz - float2(-54.0, 66.0)) < 15) ? -1 : 1);

	FragmentOutputPacket outputFragment;
	if (!USE_SHADOW_MAP)
	{
		outputFragment.fragmentColour = float4(1, 1, 1, 1);
		return outputFragment;
	}
	float3 N = normalize(v.normalW);
	float4 baseColour = diffuse;

	if (useMaterial & DIFFUSE_MAP)
		baseColour = diffMap.Sample(linearSampler, v.texCoord);



	float percentLit = 1.0f;
	if (USE_SHADOW_MAP)
		percentLit = inShadow(v, -0.001f);

	// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	float3 lightDir = -lightVec.xyz; // Directional light
	if (lightVec.w == 1.0) lightDir =  lightVec.xyz-v.posW ; // Positional light
	lightDir = normalize(lightDir);

	//Initialise returned colour to ambient component
	float3 finalColour = baseColour.xyz * lightAmbient;
	// Add diffuse light 
	finalColour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lightDiffuse * percentLit;

	// Add reflection
//	float specFactor = v.matSpecular.a;
//	specFactor = 1;
//	if (useMaterial & SPECULAR_MAP)
//		specFactor *= specMap.Sample(linearSampler, v.texCoord).r;



	// Calc specular light
	float specPower =  max(specular.a * 1000.0, 1.0f);
	float3 lightIncident = normalize(v.posW - lightVec.xyz);
	float3 eyeIncident = normalize(v.posW - eyePos  );
	float3 LR = reflect(lightIncident, N);

	// Add Code Here (Specular Factor calculation)	
	float specFactor = pow(max(dot(LR, -eyeIncident), 0.0f), specPower);
	finalColour += specFactor * specular.xyz * lightSpecular;// *min(percentLit + 0.3, 1.0);





	// Add Code Here (Calculate reflection vector ER) 
	float3 ER = reflect(eyeIncident, N);



	// Add Code Here (Sample reflected colour from envMap)
	float DNCycle = sin(Time / 5.0f);// / 2 + 0.5f;
	float3 lightFactor = saturate(float3(0.5 + (DNCycle * 0.4), 0.5 + (DNCycle * 0.4), 0.6 + (DNCycle * 0.3)) + float3(0.0, 0.0, 0.3));
	lightFactor = 1;
	float3 specColour = envMap.Sample(linearSampler, ER).xyz * float3(0.3, 0.3, 0.6) * (1 - DNCycle) * lightFactor + envMap.Sample(linearSampler, ER).xyz * (DNCycle)*lightFactor;

	//float3 specColour = envMap.Sample(linearSampler, ER).rgb;
	specColour *= specColour;//square the reflection to make it more extream

	if (useMaterial & CHROME)
		finalColour = specColour;// *2;
	else {
		// Calculate Fresnel term
		float facing = dot(N, eyeIncident);
		float _Scale = 1.0f;
		float fres = FresnelBias + _Scale * pow(1.0 + facing, FresnelExp);

		//finalColour = lerp(finalColour, specColour, fres);
		finalColour += specColour * fres;

	}















	//Add fog
	float d = length(v.posW.xyz - eyePos.xyz);
	float3 farColour =float3(0.4, 0.42, 0.4);
	//float3 farColour = (0.2 + float3(0.2, 0.22, 0.2)) * lights[0].Diffuse.rgb;
	finalColour = lerp(finalColour, farColour, min(d / 40.0 * fog, 1.0));


	outputFragment.fragmentColour = float4(finalColour,  baseColour.a);

	return outputFragment;

}
