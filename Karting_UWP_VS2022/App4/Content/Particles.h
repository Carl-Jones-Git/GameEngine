#pragma once
#include "VertexStructures.h"
#include "BaseModel.h"

#define N_PART 40
#define N_VERT N_PART*4
#define N_P_IND N_PART*6


class Texture;
class Material;
class Effect;



class Particles :public BaseModel {

protected:

	// Create Particle vertex buffer
	ParticleVertexStruct vertices[N_VERT];
	// Create the indices
	UINT indices[N_P_IND];

public:
	Particles(ID3D11Device *device, Effect *_effect, Material *_materials[], int _num_materials, ID3D11ShaderResourceView *_textures[],int _num_textures):BaseModel(device, _effect, _materials, _num_materials, _textures, _num_textures) { init(device); }
	HRESULT init(ID3D11Device *device);
	~Particles();
	void render(ID3D11DeviceContext *context);
};