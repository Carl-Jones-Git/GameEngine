#pragma once
#include <stdafx.h>
#include "VertexStructures.h"
#include <BaseModel.h>

class DXBlob;


class ParticleSystem : public BaseModel {


int N_PART = 10;
int N_VERT = N_PART * 4;
int N_P_IND = N_PART * 6;

public:
	ParticleSystem( ID3D11Device *device, shared_ptr<Effect> _effect, shared_ptr<Material> _material = nullptr) : BaseModel(device, _effect, _material){ init(device); }
	~ParticleSystem();

	HRESULT init(ID3D11Device *device);
	void render(ID3D11DeviceContext *context, int instanceIndex = 0);
};