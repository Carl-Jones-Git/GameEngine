#pragma once
#include <Effect.h>
#include <Material.h>
#include <BaseModel.h>




class Box : public BaseModel {

public:
	Box(ID3D11Device *device, shared_ptr<Effect> _effect,std::shared_ptr< Material> _material = nullptr) : BaseModel(device, _effect, _material){ init(device); }
	~Box();
	ID3D11SamplerState* anisoSampler = nullptr;
	void render(ID3D11DeviceContext *context, int instanceIndex=0);
	HRESULT init(ID3D11Device *device);

};