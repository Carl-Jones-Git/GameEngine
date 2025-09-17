// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float3 texCoord	: TEXCOORD;
};

TextureCube cubeMap : register(t0);
SamplerState linearSampler : register(s0);

// A pass-through function for the (interpolated) color data.
float4 main(PixelShaderInput input) : SV_TARGET
{

	return float4(cubeMap.Sample(linearSampler, input.texCoord).xyz, 1);
	return float4(1,0,0,1);
	return float4(input.color);
}
