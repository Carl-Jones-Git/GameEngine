
// Basic texture pixel shader

// input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct fragmentInputPacket {

	float2				texCoord	: TEXCOORD;
};


struct fragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};

Texture2D inputTex : register(t0);
Texture2D inputTex2 : register(t1);
SamplerState linearSampler : register(s0);

fragmentOutputPacket main(fragmentInputPacket inputFragment) {

	fragmentOutputPacket outputFragment;
	outputFragment.fragmentColour = inputTex.Sample(linearSampler, inputFragment.texCoord);
	//outputFragment.fragmentColour = float4(1,1,1,1);
	return outputFragment;
}
