#pragma once
#include<LookAtCamera.h>


class DynamicCube {

protected:
	LookAtCamera* cubeCameras[6];
	D3D11_VIEWPORT cubeViewport;
	ID3D11RenderTargetView* dynamicCubeMapRTV[6];
	ID3D11ShaderResourceView* dynamicCubeMapSRV = nullptr;
	ID3D11DepthStencilView* depthStencilView = nullptr;
public:
	DynamicCube(ID3D11Device* device, ID3D11DeviceContext* context, XMVECTOR pos, int size=256);
	~DynamicCube();
	void render(System *system, function<void(ID3D11DeviceContext*)> renderSceneObjects);
	void initCubeCameras(ID3D11Device* device, ID3D11DeviceContext* context, XMVECTOR pos);
	void DynamicCube::updateCubeCameras(ID3D11DeviceContext* context, XMVECTOR pos);
	ID3D11ShaderResourceView* getSRV() { return dynamicCubeMapSRV; };
};

