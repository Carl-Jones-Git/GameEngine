/*
 * Educational Game Engine - Simple Vertex Shader (No Transformation)
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

	float4				colour		: COLOR;
	float4				posH			: SV_POSITION;
};


//
// Vertex shader
//
vertexOutputPacket main(vertexInputPacket inputVertex) {

	vertexOutputPacket outputVertex;
	outputVertex.colour = inputVertex.colour;
	outputVertex.posH = float4(inputVertex.pos,1);

	return outputVertex;
}
