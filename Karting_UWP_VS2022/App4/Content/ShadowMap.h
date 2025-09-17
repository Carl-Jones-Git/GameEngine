#pragma once

#include <DirectXMath.h>
#include "Effect.h"
#include "LookAtCamera.h"
#include "CBufferStructures.h"


using namespace std;






//class Scene;

class ShadowMap
{
	// Allocate the projection matrix (it is setup in rebuildViewport).
	//projMatrixStruct* projMatrix;
	ID3D11SamplerState* sampler;
	LookAtCamera								*lightCam;
	D3D11_VIEWPORT							shadowViewport;
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T;
	XMMATRIX shadowTransform;
	// ShadowMap Views
	int										shadowMapSize = 1024;
	ID3D11ShaderResourceView				*shadowMapSRV = nullptr;
	ID3D11DepthStencilView					*shadowMapDSV = nullptr;

	Effect									*shadowEffect = nullptr;
	CBufferShadow							*cBufferCPU = nullptr;
	ID3D11Buffer							*cBufferGPU = nullptr;
public:
	ShadowMap(ID3D11Device *device, Effect *_effect, XMVECTOR lightVec, int _shadowMapSize = 1024);
	void update(ID3D11DeviceContext *context);

	void render(ID3D11DeviceContext *context, function<void(ID3D11DeviceContext*)> renderScene);
	void setShadowMatrix(ID3D11DeviceContext *context) { context->VSSetConstantBuffers(5, 1, &cBufferGPU); }
	XMMATRIX  getShadowMatrix(){ return shadowTransform; };
	ID3D11ShaderResourceView *getSRV(){ return shadowMapSRV; };
	//~ShadowMap();
};

