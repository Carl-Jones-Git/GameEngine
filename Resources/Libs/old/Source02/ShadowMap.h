#pragma once

#include <DirectXMath.h>
#include <Effect.h>
#include <LookAtCamera.h>
#include <CBufferStructures.h>


using namespace std;

//class Scene;

class ShadowMap
{
	ID3D11SamplerState* shadowSampler;
	LookAtCamera* lightCam;
	Effect* shadowEffect = nullptr;
	D3D11_VIEWPORT							shadowViewport;
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T;
	XMMATRIX shadowTransform;
	// ShadowMap Views
	int										shadowMapSize = 1024;
	ID3D11ShaderResourceView				*shadowMapSRV = nullptr;
	ID3D11DepthStencilView					*shadowMapDSV = nullptr;
	CBufferShadow							*cBufferCPU = nullptr;
	ID3D11Buffer							*cBufferGPU = nullptr;
public:
	ShadowMap(ID3D11Device *device, XMVECTOR lightVec, int _shadowMapSize = 1024);
	
	void update(ID3D11DeviceContext *context);
	ID3D11ShaderResourceView* setMapSize(ID3D11Device* device, int newMapSize);
	ID3D11ShaderResourceView* initShadowMap(ID3D11Device* device, int shadowMapSize);
	void render(System *system, function<void(ID3D11DeviceContext*)> renderSceneObjects);
	void setShadowMatrix(ID3D11DeviceContext *context) { context->VSSetConstantBuffers(5, 1, &cBufferGPU); }
	XMMATRIX  getShadowMatrix(){ return shadowTransform; };
	ID3D11ShaderResourceView *getSRV(){ return shadowMapSRV; };

	~ShadowMap();
};

