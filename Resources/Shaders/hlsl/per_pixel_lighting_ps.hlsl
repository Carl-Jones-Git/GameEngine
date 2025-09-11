
//
// Model a simple light
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

cbuffer materialCBuffer : register(b4) {

	float4 emissive;
	float4 ambient;
	float4 diffuse;
	float4 specular;
	int useMaterial;
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
	bool				isFrontFace : SV_IsFrontFace;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D specMap : register(t2);
Texture2D emissiveMap : register(t3);
SamplerState linearSampler : register(s0);

TextureCube envMap : register(t6);
Texture2D shadowMap: register(t7);
SamplerComparisonState SamShadow : register(s7);
//MaterialUsage 

static const int USE_MATERIAL_ALL = 0;
static const int DIFFUSE_MAP = 1 << 0;
static const int NORMAL_MAP = 1 << 1;
static const int SPECULAR_MAP = 1 << 2;
static const int OPACITY_MAP = 1 << 3;
static const int REFLECTION_MAP = 1 << 4;
static const int EMISSIVE_MAP = 1 << 5;
//...
static const int CHROME = 1 << 8;


static float FresnelBias = 0.0;// 0.1;//0.3;
static float FresnelExp = 1.0;//0.5;//4;



//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) { 

	FragmentOutputPacket outputFragment;
	clip((v.posW.y < 0.9f&& length(v.posW.xz - float2(-54.0, 66.0)) < 15) ? -1 : 1);

	if (!USE_SHADOW_MAP)
	{
		outputFragment.fragmentColour=float4(1,1,1,1);
			return outputFragment;
	}



	//bool useTexture = true; //Change this parameter to true to render using texture and vertex colour

	float4 baseColour = diffuse;

	if (useMaterial&DIFFUSE_MAP)
		baseColour = diffuseTexture.Sample(linearSampler, v.texCoord);




	clip(baseColour.a < 0.01f ? -1 : 1);


	float3 N = normalize(v.normalW);
	if (!v.isFrontFace)
		N = -N;
	float dist = length(v.posW.xyz - eyePos.xyz);

	if (emissive.w > 0.5f)
	{
		N = float3(0, 1, 0);
		baseColour.rgb = ((baseColour.r + baseColour.g + baseColour.b) / 1.0f) * v.matDiffuse.rgb;
		//if (dist > 5)
			baseColour.a = baseColour.a * max((30.0f - (dist)) / 30.0f, 0)*0.75;
		outputFragment.fragmentColour = baseColour;

		return outputFragment;
	}






	//Test if this pixel is in shadow
	float percentLit = 1.0;
	if (USE_SHADOW_MAP)
	{
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
			float zBias = -0.0001f;
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
	}

	//int NORMAL_MAP = 1 << 1;
	if (useMaterial & NORMAL_MAP)
	{
		float3 Nt = normalize((normalTexture.Sample(linearSampler, v.texCoord) * 2.0 - 1.0).xyz);
		// Build orthonormal basis. 
		// Transform normals from texture space to world coordinates
		// Add Code Here (Reconstruct float3X3 tangent to world matrix)
		float3 Nn = normalize(v.normalW);
		float3 T = normalize(v.tangentW);
		float3 B = cross(Nn, T);
		float3x3 TBN = float3x3(T, B, Nn);
		// Transform normal to world coordinates
		// Modify Code Here (Transform normal to world coordinates)
		// Transform from tangent space to world space. 
		float3 Nw = mul(Nt, TBN);
		N = normalize(Nw);
	}


	//Lighting
	float3 colour = 0;


	if (useMaterial & (CHROME | REFLECTION_MAP))
	{
		// Add Code Here (Calculate reflection vector ER) 
		float3 eyeIncident = normalize(v.posW - eyePos);
		float3 ER = reflect(eyeIncident, N);



		// Add Code Here (Sample reflected colour from envMap)
		float DNCycle = sin(Time /20.0f);// / 2 + 0.5f;
		float3 lightFactor = saturate(float3(0.5 + (DNCycle * 0.4), 0.5 + (DNCycle * 0.4), 0.6 + (DNCycle * 0.3)) + float3(0.0, 0.0, 0.3));
		//lightFactor = 1;
		float3 specColour = envMap.Sample(linearSampler, ER).xyz * float3(0.3, 0.3, 0.6) * (1 - DNCycle) * lightFactor + envMap.Sample(linearSampler, ER).xyz * (DNCycle)*lightFactor;

		//float3 specColour = envMap.Sample(linearSampler, ER).rgb;
		specColour *= specColour;//square the reflection to make it more extream

		if (useMaterial & CHROME)
			colour = specColour;// *2;
		else {
			// Calculate Fresnel term
			float facing = dot(N, eyeIncident);
			float _Scale = 1.0f;
			float fres = FresnelBias + _Scale * pow(1.0 + facing, FresnelExp);

			//finalColour = lerp(finalColour, specColour, fres);
			colour += specColour;// *fres;

		}
	}

	////Initialise returned colour to ambient component
	//colour += baseColour.xyz * lights[0].Ambient;

	//// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	//float3 lightDir = -lights[0].Vec.xyz; // Directional light
	//if (lights[0].Vec.w == 1.0)
	//{
	//	float d = length(v.posW.xyz - lights[0].Vec.xyz);
	//	//att = 1.0 / (d * d * lights[i].Attenuation.z + d * lights[i].Attenuation.y + lights[i].Attenuation.x);
	//	lightDir = lights[0].Vec.xyz - v.posW; // Positional light
	//}
	//lightDir = normalize(lightDir);
	//// Add Code Here (Add diffuse light calculation)
	//colour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lights[0].Diffuse;
	//outputFragment.fragmentColour = float4(colour, 1);
	//return outputFragment;

	for (int i = 0; i < numLights; i++)
	{
		if (length(v.posW - lights[i].Vec.xyz) > lights[i].Attenuation.w)
			continue;

		if (lights[i].Cone.w > 0.0f)
			if (dot(normalize(lights[i].Cone.xyz), normalize(v.posW - lights[i].Vec.xyz)) < lights[i].Cone.w)
				continue;

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
		//pLit = 1.0;
		//att = 1.0;
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







	//outputFragment.fragmentColour = float4(colour, 1); //);
	//return outputFragment;
	//add fog
	float d = length(v.posW.xyz - eyePos.xyz);
	//float3 farColour = colour * 0.2 + float3(0.2, 0.22, 0.2);
	float3 farColour = (0.2 + float3(0.2, 0.22, 0.2)) * lights[0].Diffuse.rgb;
	colour = lerp(colour, farColour, min(d / 40.0 * fog, 1.0));

	outputFragment.fragmentColour = float4(colour, baseColour.a); //);
	return outputFragment;

}
