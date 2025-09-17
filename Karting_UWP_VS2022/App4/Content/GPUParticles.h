#pragma once
#include "VertexStructures.h"
#include "Particles.h"

class GPUParticles : public Particles{

	ID3D11Buffer					*vertexBuffer2 = nullptr;
	Effect							*updateEffect= nullptr;

public:

	GPUParticles(ID3D11Device *device, Effect *_effect, Effect *_updateEffect, Material *_materials[], int _num_materials, ID3D11ShaderResourceView *_textures[],int _num_textures):Particles(device, _effect, _materials, _num_materials,   _textures, _num_textures) { init(device,_updateEffect); }
	HRESULT init(ID3D11Device *device, Effect *_updateEffect);
	void render(ID3D11DeviceContext *context);
	void setWorldMatrix(XMMATRIX _worldMatrix);
	void updateParticles(ID3D11DeviceContext *context);
};