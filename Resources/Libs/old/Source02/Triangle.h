#pragma once
#include <Effect.h>
#include <Material.h>
#include <BaseModel.h>




class Triangle : public BaseModel {

public:
	Triangle(ID3D11Device *device, shared_ptr<Effect> _effect, shared_ptr<Material>_material=nullptr) : BaseModel(device, _effect, _material){ init(device); }
	~Triangle();

	void render(ID3D11DeviceContext *context, int instanceIndex = 0);
	HRESULT init(ID3D11Device *device);

};