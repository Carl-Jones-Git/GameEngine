
/*
 * Educational Game Engine - Copy Depth Pixel Shader used with Screen Quad
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */

cbuffer filterCBuffer : register(b0) {

	int		ImageWidth;
	int		ImageHeight;
	float	GaussWidth;
};

// input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct fragmentInputPacket {

	float2				texCoord	: TEXCOORD;
	float4				posH		: SV_POSITION;
};


struct fragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
	float			fragmentDepth : SV_DEPTH;
};

//
// Textures
//

// Assumes texture bound to texture t0 and sampler bound to sampler s0
Texture2DMS  <float>depthTexture: register(t0);
SamplerState linearSampler : register(s0);

fragmentOutputPacket main(fragmentInputPacket inputFragment) {

	fragmentOutputPacket outputFragment;
	float zBufDepth = depthTexture.Load(int4(inputFragment.texCoord.x * ImageWidth, inputFragment.texCoord.y * ImageHeight, 0, 0), 0).r;
	outputFragment.fragmentColour = float4(0, 0, 0, 0.1);
	outputFragment.fragmentDepth = zBufDepth;
	return outputFragment;
}
