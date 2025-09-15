/*
 * Educational Game Engine - Simple Texture Shader
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */

//----------------------------
// Input / Output structures
//----------------------------
struct vertexInputPacket {

	float3				pos			: POSITION;
	float4				colour		: COLOR;
};

struct vertexOutputPacket {

	float2				texCoord	: TEXCOORD;
	float4				posH		: SV_POSITION;

};

//
// Vertex shader
//
vertexOutputPacket main(vertexInputPacket inputVertex) {

	vertexOutputPacket outputVertex;

	outputVertex.texCoord = inputVertex.pos.xy*0.5+0.5;
	outputVertex.texCoord.y=1.0f- outputVertex.texCoord.y;
	outputVertex.posH = float4(inputVertex.pos.xy,0.9,1.0);

	return outputVertex;
}
