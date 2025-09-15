

//
// Convolve V Shader modified by Carl Jones 2023 from:
//https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-14-advanced-techniques-realistic-real-time-skin
//

// Ensure matrices are row-major
#pragma pack_matrix(row_major)

cbuffer filterCBuffer : register(b0) {


	int		ImageWidth;
	int		ImageHeight;
	float	GaussWidth;
};

//-----------------------------------------------------------------
// Structures and resources
//-----------------------------------------------------------------

//
// Textures
//

// Assumes texture bound to t0 and sampler bound to sampler s0
// inputTex – Texture being convolved
Texture2D  inputTex:register(t0);
SamplerState linearSampler : register(s0);




//-----------------------------------------------------------------
// Input / Output structures
//-----------------------------------------------------------------

// Input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct FragmentInputPacket {
	float2				texCoord		: TEXCOORD;
	float4				posH			: SV_POSITION;
};


struct FragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};


//-----------------------------------------------------------------
// Pixel Shader - Lighting 
//-----------------------------------------------------------------

FragmentOutputPacket main(FragmentInputPacket IN) {

	FragmentOutputPacket outputFragment;

	float scaleConv = 1.0 / ImageHeight;

	float netFilterWidth = scaleConv * GaussWidth;
	// Gaussian curve – standard deviation of 1.0
	float curve[7] = { 0.006, 0.061, 0.242,  0.383,  0.242, 0.061,  0.006 };
	float2 coords = IN.texCoord - float2(0.0, netFilterWidth * 3.0);
		float4 sum = 0;
		for (float i = 0; i< 7; i++)
		{
			float4 tap = inputTex.Sample(linearSampler, coords);
				sum += curve[i] * tap;
			coords += float2(0.0,netFilterWidth);
		}
	
	outputFragment.fragmentColour = float4(sum.rgb,1);
	return outputFragment;
}
