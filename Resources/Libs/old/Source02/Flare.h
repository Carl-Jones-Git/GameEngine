#pragma once
#include<Utils.h>
#include <BaseModel.h>
#include<Effect.h>
#include<VertexStructures.h>

class Flare : public BaseModel {

protected:
	bool visible = true;
	CBufferTextSize* cBufferTextSizeCPU = nullptr;
	ID3D11Buffer* cBufferTextSizeGPU = nullptr;
	void Flare::updateTextSize(ID3D11DeviceContext* context, int width, int height);
	void Flare::initCBuffer(ID3D11Device* device, int _texWidth = 256, int _texHeight = 256);
public:
	Flare(XMFLOAT3 position, ID3D11Device *device, shared_ptr<Effect> _effect, shared_ptr<Material> _material = nullptr) : BaseModel(device, _effect, _material){ init(device, position); }
	~Flare();
	void render(ID3D11DeviceContext *context, int instanceIndex = 0);
	void render(System* system);
	HRESULT init(ID3D11Device *device, XMFLOAT3 position);
	HRESULT init(ID3D11Device *device){ return S_OK; };
};

