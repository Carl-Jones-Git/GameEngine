
/*
 * Educational Game Engine - Grass/Ground Effect - Modified Shell Shader
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

//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2D diffuseTexture : register(t0);
Texture2D asphaltTexture : register(t1);
Texture2D grassAlpha: register(t2);
Texture2D colourMap: register(t3);
Texture2D normalMap: register(t4);
Texture2D noiseMap: register(t5); 
TextureCube envMap : register(t6);
Texture2D shadowMap: register(t7);
Texture2D noise2Map: register(t8);

SamplerState anisotropicSampler : register(s0);
SamplerComparisonState SamShadow : register(s7);



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


float inShadow(FragmentInputPacket v, float zBias = 0.0001f)
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
// Grass Shell Shader
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) {

	FragmentOutputPacket outputFragment;

	bool applySpec = false;
	bool applyRefl = false;

	//Get the distace from this pixel to the viewer
	float dist = length(v.posW.xyz - eyePos.xyz);
	float distFadeStart = 5;
	float distFadeRange = 5 ;
	clip((dist > distFadeStart+distFadeRange && grassHeight > 0.0)? -1 : 1);//Clip if we are rendering a shell >0 and distace from the camera is > 10

	float tileRepeat = 48;//tile repeate for the grass alpha texture used for the shell alpha
	
	//Sample the base colour from the main terrain texture. It is a single large texture spanning the entire terrain.It is green where there is grass and grey where there is track.
	float3 baseColour= diffuseTexture.Sample(anisotropicSampler, v.texCoord);
	float3 baseColour2 = diffuseTexture.Sample(anisotropicSampler, v.texCoord + 0.0004);// we use multiple samples to help antialise the line between track and grass
	float3 baseColour3 = diffuseTexture.Sample(anisotropicSampler, v.texCoord - 0.0004);
	float3 specAmount = 0.3;
	float specPower = 2;
	float alpha = 1.0;
	float3 N = normalize(v.normalW);
	float normalMapWeight = 0.3;

	//Check if we are on track or grass. If green is low and blue is high we are on the track
	if ((baseColour.g < 0.06 || baseColour.b > 0.6 )|| (baseColour2.g < 0.06 || baseColour2.b > 0.6) || (baseColour3.g < 0.06 || baseColour3.b > 0.6))
	{
		clip((grassHeight > 0.0) ? -1 : 1);//if this pixel is track then we dont render grass shells so clip if shell>0
		applySpec = true;

		//Add texture detail from the asphaltTexture to the basecolour 
		if (baseColour.b > 0.2 || baseColour2.b > 0.2 || baseColour3.b > 0.2)//White line
		{
			baseColour = baseColour * 0.4 * (1.0 - dist / 50.0f) + asphaltTexture.Sample(anisotropicSampler, v.texCoord * 100);
			//Darken Colour Slightly
			baseColour *= 0.7;
		}
		else//Track Colour
		{
			//Resample the asphaltTexture at different scales to avoid obvious repetetive tiling
			baseColour = baseColour + asphaltTexture.Sample(anisotropicSampler, v.texCoord * 100) * 0.9;
			//Darken Colour Slightly
			baseColour *= 0.7;
		}
	}
	else
	{
		//Grass Colour
		baseColour = baseColour * 0.7 + colourMap.Sample(anisotropicSampler, v.texCoord * 200) * 0.3;
		baseColour.g = max(baseColour.g, 0.3);
		
		//Fade to Mud Colour if we are near water center (-54.0, 1.0, 66.0)
		if (length(v.posW.xyz - float3(-54.0, 1.0, 66.0)) < 15)
		{
			//Fade from Grass colour to mud colour near water
			float fadeTop = 1.1;
			float fadeBottom = 1.0;
			float fadeRange = fadeTop - fadeBottom;
			float waterLineTop = 0.95;
			float waterLineBottom = 0.9;
			float waterLineRange = 0.05;

			clip(grassHeight > 0.0 && v.posW.y < (fadeBottom) ? -1 : 1);
			float3 mudcolour = float3(0.111, 0.0855, 0.053) * asphaltTexture.Sample(anisotropicSampler, v.texCoord * 5).rgb +0.22;

			if (v.posW.y > waterLineTop && v.posW.y < fadeTop)
			{
				//fade from grass to mud
				float w = saturate((v.posW.y - fadeBottom) / fadeRange);
				if (grassHeight > 0.0)
					alpha = lerp(0, 1, w);
				else
				{
					baseColour = lerp(mudcolour, baseColour, w);
				}		
			}

			else if (v.posW.y <= waterLineTop)
			{
				applySpec = true;
				applyRefl = true;
				specAmount = 0;
				specPower = 5;
				float w = max((v.posW.y - waterLineBottom) / (waterLineRange), 0);
				baseColour =  lerp(mudcolour * 0.8, mudcolour, saturate((w - 0.5) * 2));

				if (w <= 0.5)
				{
					specAmount = lerp(0.25, specAmount, saturate(w * 2));
				}
			}
		}

	}
	if (applySpec)
	{
		//Get the normals froma normal map. 
		float3 Nt = normalize((normalMap.Sample(anisotropicSampler, v.texCoord * 100) * 2 - 1.0)).xyz; 
		float3 T = normalize(v.tangentW);
		float3 B = cross(N, T);
		float3x3 TBN = float3x3(T * 0.5,B*0.5,N);

		// Modify Code Here (Transform normal to world coordinates)
		// Transform from tangent space to world space. 
		float3 Nw = mul(Nt, TBN);	
		float fadeFactor = min(max(1.0f - (dist - distFadeStart) / distFadeRange, 0), 1);//Blend the texture normals with the vertex normals. Might be simpler to use a tangent matrix?
		N = normalize(lerp(N, normalize(Nw), fadeFactor));// normalMapWeight));
	}

	//Test if this pixel is in shadow
	float percentLit = 1.0;
	if (USE_SHADOW_MAP)
		percentLit = inShadow(v);

	//Lighting
	float3 colour = 0;
	float3 eyeDir = normalize(eyePos - v.posW);
	float3 RE = reflect(-eyeDir, N);
	for (int i = 0; i < numLights; i++)
	{
		if (length(v.posW - lights[i].Vec.xyz)>lights[i].Attenuation.w  )
			continue;

		if (lights[i].Cone.w > 0.0f)
			if (dot(normalize(lights[i].Cone.xyz), normalize(v.posW-lights[i].Vec.xyz )) < lights[i].Cone.w)
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

		lightDir = normalize(lightDir);

		//Initialise returned colour to ambient component
		colour += baseColour.xyz * lights[i].Ambient * att;;

		// Add Code Here (Add diffuse light calculation)
		colour += max(dot(lightDir, N), 0.0f) * baseColour.xyz * lights[i].Diffuse * pLit * att;

		//Add specular lighting for the track
		if (applySpec)
		{
			float3 R = reflect(-lightDir, N);
			//Specular Factor calculation
			float specFactor = pow(max(dot(R, eyeDir), 0.0f), specPower);
			colour += max(specFactor * lights[i].Specular *  specAmount * pLit * att, 0);// ;
		}
	}
	// Add Code Here (Sample reflected colour from envMap)
	if (applyRefl)
	{
		float DNCycle = sin(Time / 5.0f);// / 2 + 0.5f;
		float3 lightFactor = saturate(float3(0.5 + (DNCycle * 0.4), 0.5 + (DNCycle * 0.4), 0.6 + (DNCycle * 0.3)) + float3(0.0, 0.0, 0.3));
		lightFactor = 1;
		float3 specColour = envMap.Sample(anisotropicSampler, RE).xyz * float3(0.3, 0.3, 0.6) * (1 - DNCycle) * lightFactor + envMap.Sample(anisotropicSampler, RE).xyz * (DNCycle)*lightFactor;
		colour += max(specColour * min(percentLit + 0.3, 1.0) * specAmount, 0); //*specFactor / 2
	}


	if (!applySpec)
		if (grassHeight > 0.0)//We are rendering a grass shell
		{
			float fadeFactor = 1.0f - (dist - distFadeStart) / distFadeRange;
			float qualityFactor = max(0.7 - QUALITY / 7.0f, 0);
			float relativeGrassHeight = grassHeight * 1800.0f;//grass length reletive to max length
			alpha *= grassAlpha.Sample(anisotropicSampler, v.texCoord * tileRepeat).a + qualityFactor;
			alpha *= noiseMap.Sample(anisotropicSampler, v.texCoord * tileRepeat).r + qualityFactor;

			// Reduce alpha and increase illumination for tips of grass
			// max grass height is approx 1/600
			alpha = (alpha - relativeGrassHeight*0.25);
			clip((alpha < 0.1f) ? -1 : 1);
			{
				float3 tipColour = lights[0].Diffuse.rgb * float3(0.025, 0.05, 0);
				colour = lerp(colour, lerp(colour - tipColour, colour + tipColour, relativeGrassHeight+(1.0- noiseMap.Sample(anisotropicSampler, v.texCoord * tileRepeat).r)), fadeFactor);
			}
		}

	//Add fog
	float d=length(v.posW.xyz - eyePos.xyz);
	float3 farColour=(0.2+float3(0.2, 0.22, 0.2)) * lights[0].Diffuse.rgb;
	colour = lerp(colour, farColour, min(d / 40.0*fog, 1.0));

	outputFragment.fragmentColour = float4(colour, alpha);
	return outputFragment;

}
