
// Ensure matrices are row-major
#pragma pack_matrix(row_major)
//
// Model the update of each particle.  Each particle is modelled in world coordinate space
//

// Resources
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
	float4x4			worldIRMatrix; // Correctly transform normals to world space
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
	float4						windDir;
	float						Time;
	float						grassHeight;
	float						deltaTime;
	bool				USE_SHADOW_MAP;
	bool				REFLECTION_PASS;
};

Texture2D myTexture : register(t0);
SamplerState textureSampler : register(s0);


// Input / Output structures


struct ParticleStructure {
		float3 pos : POSITION;   // in object space
		float3 posL : LPOS;   // in object space
		float3 vel :VELOCITY;   // in object space
		float3 data : DATA;
};


//One particle in maps to 1 paricleout
[maxvertexcount(1)]
void main(point ParticleStructure inputParticle[1],
	inout PointStream<ParticleStructure> outputParticleStream) {

	ParticleStructure			outputParticle = inputParticle[0];

	//tweakable parameters
	float gPartLife = 1.2;//seconds
	float3 gPartSpeed = float3(0.5f,2.0f,0.5f);

	float dtime =  deltaTime;
	float3 g = float3(0, -0.8, 0);
	float3 wind = windDir.xyz*0.0;

	// Calculate force
	float3 force = g + wind; 
	force = mul(float4(force, 1), worldIRMatrix);


	outputParticle.data.x += dtime; //increment age
	if (outputParticle.data.x > 0.0)
	{
		//float3 worldVel = mul(float4(inputParticle[0].vel, 1),worldMatrix);
		outputParticle.vel = inputParticle[0].vel + dtime * force; //Update velocity
		//worldVel = worldVel + dtime*force; //Update velocity in world space

		//float3 worldPos= mul(float4(inputParticle[0].pos, 1), worldMatrix);
		outputParticle.pos = inputParticle[0].pos + (outputParticle.vel* dtime*gPartSpeed); // Update position
		//worldPos = worldPos + (worldVel* dtime*gPartSpeed); // Update position
	}


	////Check if particle hits floor and bounce
	//if (worldPos.y<0)
	//{
	//	worldPos.y = -worldPos.y;
	//	worldVel.y = -worldVel.y*0.3;
	//	outputParticle.data.x += 0.1;
	//}


	//outputParticle.pos = mul(float4(worldPos, 1), worldITMatrix);
	//outputParticle.vel = mul(float4(worldVel, 1), worldITMatrix);

	
	
	//Check if particle has expired and respawn
	if (outputParticle.data.x > gPartLife)
	{
		outputParticle.data.x = 0.0;// fmod(outputParticle.data.x, gPartLife);
		outputParticle.vel = outputParticle.posL;// Copy back original velocity
		//outputParticle.vel.y = 0.7+(abs(outputParticle.vel.x) + abs(outputParticle.vel.z))/2;
		outputParticle.pos = float3(0, 0, 0);
	}
	


	outputParticleStream.Append(outputParticle);
}
