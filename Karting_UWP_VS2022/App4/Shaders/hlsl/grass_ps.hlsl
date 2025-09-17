
//
// Grass effect - Modified a fur technique
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)


//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

//-----------------------------------------------------------------
//// Globals
////-----------------------------------------------------------------



//-----------------------------------------------------------------
// Globals
//-----------------------------------------------------------------
struct Light {
	float4				Vec; // w=1: Vec represents position, w=0: Vec  represents direction.
	float4				Ambient;
	float4				Diffuse;
	float4				Specular;
	float4				Attenuation;// x=constant,y=linear,z=quadratic,w=cutOff
};
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
	Light light[2];
};
cbuffer sceneCBuffer : register(b3) {
	float4				windDir;
	float				Time;
	float				grassHeight;
	bool				USE_SHADOW_MAP;
	bool				REFLECTION_PASS;
}

//
// Textures
//


Texture2D myTexture : register(t0);
Texture2D grassAlpha: register(t1);
TextureCube envMap: register(t3);
Texture2D grassNormals: register(t2);
Texture2D shadowMap: register(t4);
SamplerState mySampler : register(s0);
SamplerComparisonState SamShadow : register(s1);



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



float3 lambertian(int lightNum, float3 baseColour, float3 N, float3 posW,float diffuse, float specMaterialAmount, float3 specMaterialColour)
{
	// Calculate the lambertian term (essentially the brightness of the surface point based on the dot product of the normal vector with the vector pointing from v to the light source's location)
	float3 lightDir = -light[lightNum].Vec.xyz; // Directional light
	if (light[lightNum].Vec.w == 1.0) lightDir = light[lightNum].Vec.xyz- posW  ; // Positional light
	float att = 1.0;
	float dist = length(lightDir);
	lightDir = normalize(lightDir);

	float cutOff = light[lightNum].Attenuation.w;
	if (dist < cutOff)
	{
		float con = light[lightNum].Attenuation.x; float lin = light[lightNum].Attenuation.y; float quad = light[lightNum].Attenuation.z;
		att = 1.0 / (con + lin * dist + quad * dist*dist);
	}
	else
		return float3(0, 0, 0);

	float FresnelBias = 0.4;//0.3;
	float FresnelExp = 8.0;//4;

	float3 colour =baseColour*saturate(dot(N, lightDir))*light[lightNum].Diffuse*diffuse*att;

	// Calculate the specular term
	float specPower =max(specMaterialAmount*1000.0, 1.0f);
	float3 eyeDir = normalize(eyePos.xyz- posW  );
	float3 R = reflect(-lightDir, N);
	float specFactor = pow(saturate(dot(R, eyeDir)), specPower);
	//colour+= specFactor * specMaterialColour*att;
	// Calculate Fresnel term
	float facing = 1 - max(dot(N, eyeDir), 0);
	//float fres = (FresnelBias + (1.0 - FresnelBias)*pow(abs(facing), abs(FresnelExp)));
	float fres = (FresnelBias + (1.0 - FresnelBias)*pow(facing, FresnelExp));;
	colour += specFactor * specMaterialColour*att*fres;
	//colour += specFactor * specMaterialColour*att;
	//if (specPower > 1)
	{
		specPower = specMaterialAmount * 15;
			// Add Code Here (Calculate reflection vector ER) 
			float3 ER = reflect(-eyeDir, N);
			float3 specColour = specPower * envMap.Sample(mySampler, ER).rgb*specMaterialColour;



			//colour = (colour*(1 - fres)) + (fres * specColour);
			colour += (fres * specColour);
			//colour += specColour;
			//colour = fres;
	}




	return colour;// float3(1, 1, 1)*dot(R, -eyeDir);
}
//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket v) {
	float maxHeight = 0.01;
	float grassStage = (1.0 / maxHeight)*grassHeight;//0...1
	float tileRepeat = 64;


	FragmentOutputPacket outputFragment;

	//Fade from grass to dry mud and then to wet mud

	// Grass
	//--------------------------grassFadeTop = 0.3
	//	Fade from grass to mud
	//--------------------------grassFadeBottom =0.01
	// Mud

	float grassFadeTop = 0.3;//-0.2;
	float grassFadeBottom = 0.01;//-0.95;
	float fadeRange = grassFadeTop - grassFadeBottom;




	float alpha = 1.0;
	
	
	float3 N = normalize(v.normalW);


	float distance = length(eyePos.xyz - v.posW);

	//if(grassStage>0.0&&distance < 5)
	clip(grassStage>0.0&&distance > 10 ? -1 : 1);

	clip(grassStage>0.0&&v.posW.y < grassFadeBottom ? -1 : 1);


	int num_samples = 1;

	float3 baseColour = myTexture.Sample(mySampler, v.texCoord);




	//oversample grass texture when it is close to to the viewer 
	//if (distance < 20)
	//{
	//	baseColour += myTexture.Sample(anisotropicSampler, v.texCoord * 10);
	//	num_samples++;
	//}

	
	float distanceFadeMaxEnd = 10;
	float distanceFadeMinStart = 5;
	float distanceFadeRange = distanceFadeMaxEnd - distanceFadeMinStart;
	float3 hiResBumps;
	if (distance < distanceFadeMaxEnd * 2)
	{
		float3 hiRes =lerp(myTexture.Sample(mySampler, v.texCoord * tileRepeat*0.5), baseColour, 0.5);
	
		float w = (distance - distanceFadeMaxEnd) / (distanceFadeRange*2);
		baseColour = lerp(hiRes, baseColour, w);

		if (distance < distanceFadeMaxEnd)
		{

			// Build orthonormal basis. 
			// Transform normals from texture space to world coordinates
			// Add Code Here (Reconstruct float3X3 tangent to world matrix)

			//float3 T = normalize(v.tangentW - dot(v.tangentW, N)*N);
			float3 T = normalize(v.tangentW);
			float3 B = normalize(cross(N, T));
			float3x3 TBN = float3x3(T, B, N);
			// Transform normal to world coordinates
			// Modify Code Here (Transform normal to world coordinates)
			// Transform from tangent space to world space. 

			//float3 Nt = normalize(grassNormals.Sample(anisotropicSampler, v.texCoord * tileRepeat).xyz*float3(0.5,1.0,0.5));
			//float3 Nt = fl0at3(0,)normalize(grassNormals.Sample(anisotropicSampler, v.texCoord * tileRepeat*0.5).xyz);
			float3 Nt = (grassNormals.Sample(mySampler, v.texCoord * tileRepeat)*2-0.5).xyz;
	
			//Nt = normalize(Nt);
			//Nt =lerp( float3(0, 0, 1),Nt, 0.5);
			float3 Nw = mul(Nt, TBN);
			float3 Nn = normalize(Nw);

			hiResBumps = Nn;// (Nw + 1) / 2;// Nn;// lerp(Nn, N, 0.2);

			///N = hiResBumps;

			//float3 hiResBumps = lerp(normalize(grassNormals.Sample(anisotropicSampler, v.texCoord * tileRepeat).yzx),N,0.2);
			//N = normalize(hiResBumps + N);
			
			hiRes= lerp(myTexture.Sample(mySampler, v.texCoord * tileRepeat), baseColour, 0.5);
			//hiRes = float3(0.5, 0.5, 0.5);
			//baseColour = float3(0.5, 0.5, 0.5);
			//hiRes = Nn;
			if (distance > distanceFadeMinStart)
			{
				 w = (distance - 5) / distanceFadeRange;
				 
				baseColour=lerp(hiRes, baseColour,  w);
				//N= lerp(hiResBumps, N, w);
				//baseColour = baseColour*w +hiRes*(1-w);
			}
			else if(v.posW.y< grassFadeTop)
			{
				baseColour = hiRes;
				N = lerp(hiResBumps, N, min((distance*distance /25)+0.2,1.0));// (distance / distanceFadeMinStart)*(distance / distanceFadeMinStart));
				
																			  
																			  
																			  //N = lerp(hiResBumps, N, 0.5);// (distance / distanceFadeMinStart)*(distance / distanceFadeMinStart));
				//N=normalize(N);
				//N = hiResBumps;
			}
			//N = normalize(Nn);
			//N = lerp(hiResBumps, N, 0);
			//N = normalize(N);
		}
	}




	//float3 colourx = lambertian(float3(1, 0, 0), N, v.posW, 0.1, float3(1, 1, 1));

	//outputFragment.fragmentColour = float4(N, alpha);
	//return outputFragment;

	

	baseColour*=v.matDiffuse.xyz;
	float4 specColour = v.matSpecular;

	//specColour = float4(0.5, 0.5, 0.5, 0.05);;
	//float3 colour1 = lambertian(baseColour, N, v.posW, specColour.a, specColour.xyz);
	////specColour = float4(0.0, 0.0, 0.5, 0.05);;
	////colour1 += lambertian(baseColour, N, v.posW, specColour.a, specColour.xyz);
	////colour += lambertian(baseColour, N, v.posW, specColour.a, specColour.xyz);

	//outputFragment.fragmentColour = float4(colour1, alpha);
	//return outputFragment;

	
	if (v.posW.y < grassFadeTop)
	{
		float3  mudcolour = float3(222, 171, 106) / 100.0;// / float3(159, 125, 98) / 255.0;
		mudcolour *= (baseColour.r + baseColour.g + baseColour.b) / 3.0;// greyscale of grass to get consistent texture
		if (v.posW.y > grassFadeBottom)
		{
			//fade from grass to mud
			float w = (v.posW.y - grassFadeBottom) / fadeRange;
			baseColour = lerp(mudcolour, baseColour, w);
		}
		else
			baseColour = mudcolour;
	}











	if (REFLECTION_PASS)
		clip(v.posW.y < -1.0f ? -1 : 1);






	// near the waterlevel wet mud is more specular and id darker

	//Dry Mud
	//---------------- waterSeep = 0.1;
	//Fade from dry mud to wet mud
	//---------------- waterLevel = 0.0;
	//Wet Mud

	float waterLevel = 0.0;
	float waterSeep = 0.05;
	specColour = float4(0.05, 0.05, 0.05, 0.0001);

	//Darken terrain if it is close to or below the waterline
	float wetMudDarken = 0.6;
	float4 wetMudSpecular=float4(0.9, 0.9, 0.9, 0.05);
	if (v.posW.y > waterLevel)
	{
		if (v.posW.y < waterLevel + waterSeep)
		{
			float w = (v.posW.y - waterLevel) / waterSeep;
			baseColour= (baseColour*w)+ (baseColour*wetMudDarken)*(1-w);
			specColour = (specColour*w) + (wetMudSpecular)*(1 - w);
			//specColour = wetMudSpecular;
		}
		else if(grassStage>0.0|| (distance > distanceFadeMaxEnd))
			specColour = float4(0.1, 0.1, 0.1, 0.01);

		
	}
	else
	{
		float w = (v.posW.y - waterLevel) / -0.9;
		baseColour *= wetMudDarken*(1-w);// wet mudcolour is darker;
		specColour = wetMudSpecular*w*0.2;
		specColour = wetMudSpecular;
	}


	float3 lightDir = light[0].Vec.xyz - v.posW;
	float dist = length(lightDir);
	float att = 1.0;
	float3 colour = float3(0, 0, 0);

		if (dist < light[0].Attenuation.w)
		{
			float con = light[0].Attenuation.x; float lin = light[0].Attenuation.y; float quad = light[0].Attenuation.z;
			att = 1.0 / (con + lin * dist + quad * dist*dist);

	// Apply lighting
	// Apply ambient lighting


	colour = baseColour* v.matDiffuse*light[0].Ambient.xyz*att;
	//N = float3(0,1,0);


	float3 lamb = lambertian(0,baseColour, N, v.posW, v.matDiffuse, specColour.a, specColour.xyz);

	//float3 lamb = lambertian(baseColour, N, v.posW, v.matSpecular.a, v.matSpecular.xyz);
	//float3 lamb = float3(1, 1, 1);
	v.posSdw.xyz /= v.posSdw.w;// Complete projection by doing division by w.
	// Check if pixel is outside of the bounds of the shadowmap
	if (v.posSdw.x < 1 && v.posSdw.x>0 && v.posSdw.y < 1 && v.posSdw.y>0)
	{
		float percentLit = 1.0;
		percentLit = shadowMap.SampleCmp(SamShadow, v.posSdw.xy, v.posSdw.z);
		//float depthShadowMap = shadowMap.Sample(anisotropicSampler, v.posSdw.xy).r;
		//if (v.posSdw.z >= (depthShadowMap)) percentLit = 0.0;

		lamb *= min(percentLit + 0.5, 1.0);

	}
	colour += lamb;
		}
	lightDir = light[1].Vec.xyz - v.posW;
	 dist = length(lightDir);
	 if (dist < light[1].Attenuation.w)
	 {
	//	 baseColour = float3(1, 1, 1);
	 //v.matDiffuse = float4(1, 1, 1,1);	 
		 float con = light[1].Attenuation.x; float lin = light[1].Attenuation.y; float quad = light[1].Attenuation.z;
		 att = 1.0 / (con + lin * dist + quad * dist*dist);
	 }


	if (light[1].Attenuation.w >dist)
	{
		colour+= baseColour * v.matDiffuse.xyz*light[1].Ambient.xyz*att;
		colour += lambertian(1, baseColour, N, v.posW, v.matDiffuse, specColour.a, specColour.xyz);
	}
	//else colour = float3(0, 0, 0);

//	colour += lambertian(baseColour, N, v.posW, specColour.a, specColour.xyz);
	//colour += lambertian(baseColour, N, v.posW, specColour.a, specColour.xyz);

	//outputFragment.fragmentColour = float4(colour, alpha);
	//return outputFragment;

	//colour = lambertian(float3(1, 1, 1), N, v.posW, 0.1, float3(1, 1, 1));
	//outputFragment.fragmentColour = float4(colour, 1);
	//return outputFragment;

	////Test if this pixel is in shadow
	//if (USE_SHADOW_MAP)
	//{
	//	v.posSdw.xyz /= v.posSdw.w;// Complete projection by doing division by w.
	//	// Check if pixel is outside of the bounds of the shadowmap
	//	if (v.posSdw.x < 1 && v.posSdw.x>0 && v.posSdw.y < 1 && v.posSdw.y>0)
	//	{
	//		float depthShadowMap = shadowMap.Sample(anisotropicSampler, v.posSdw.xy).r;
	//		// Test if pixel depth is >= the value in the shadowmap(i.e behind/obscured) 
	//		if (v.posSdw.z >= (depthShadowMap + 0.00001)){//add small bias to avoid z fighting
	//			// If this pixel is behind the value in the shadowmap it is in shadow
	//			// so return ambient only. If not complete full lighting calculation 
	//			//outputFragment.fragmentColour = float4(colour, alpha);
	//			//return outputFragment;
	//			colour *= 1.7;

	//		}
	//		else
	//			colour += lambertian(baseColour, N, v.posW, v.matSpecular.a, v.matSpecular.xyz);

	//	}
	//	else
	//		colour += lambertian(baseColour, N, v.posW, v.matSpecular.a, v.matSpecular.xyz);
	//}
	//else
	//	colour += lambertian(baseColour, N, v.posW, specColour.a, specColour.xyz);




	// only render grass that is less than distanceFadeMaxEnd
	if (grassStage > 0.0)
	{
		//colour = float3(1, 1, 1);
		alpha = grassAlpha.Sample(mySampler, v.texCoord*tileRepeat*0.5).a;
		// Alpha Test
		clip(alpha < 0.5f ? -1 : 1);// clip for alpha < 0.9
		
		// Reduce alpha
		alpha = (grassStage);


		//fade out grass that is below upperFadeStart
		if (v.posW.y - grassFadeBottom)
		{
			float w = (v.posW.y - grassFadeBottom) / fadeRange;
			alpha = lerp(0, alpha, w);
		}
		alpha = min(alpha, 0.1);

		//fade out grass that is further than distanceFadeMinStart and less than distanceFadeMaxEnd
		if (distance > distanceFadeMinStart)
		{
			float w = (distance - distanceFadeMinStart) / distanceFadeRange;
			alpha = lerp( alpha,0, w);
		}

		// and increase illumination for tips of grass
		colour *= 20*alpha;// alpha * 5;// (grassStage + 1.0);


	}
	//if (grassStage > 0.0 && v.posW.y < grassFadeBottom)
	//	clip(-1);

	outputFragment.fragmentColour = float4(colour,alpha);
	return outputFragment;

}
