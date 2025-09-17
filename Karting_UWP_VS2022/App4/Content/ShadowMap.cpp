#include <pch.h>
#include "ShadowMap.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ShadowMap::ShadowMap(ID3D11Device *device, Effect *_effect, XMVECTOR lightVec, int _shadowMapSize)
{
	shadowMapSize = _shadowMapSize;
	shadowEffect = _effect;

	ID3D11RasterizerState* shadowRSState=shadowEffect->getRasterizerState();
	D3D11_RASTERIZER_DESC RDesc;
	shadowRSState->GetDesc(&RDesc);
	RDesc.DepthBias = 100000;
	RDesc.DepthBiasClamp = 0.0f;
	RDesc.SlopeScaledDepthBias = 1.0f;
	device->CreateRasterizerState(&RDesc, &shadowRSState);
	shadowEffect->setRasterizerState(shadowRSState);

	
	

	//projMatrix = (projMatrixStruct*)_aligned_malloc(sizeof(projMatrixStruct), 16);
	//ProjMatrix->projMatrix = XMMatrixPerspectiveFovLH(0.5f*XM_PI, 1.0f, 0.5f, 1000.0f);
	//projMatrix->projMatrix = XMMatrixPerspectiveFovLH(3.142f * 0.35f, 1, 0.1f, 500.0f);

	//Setup light camera
	XMFLOAT3 upf = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3 targetf = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 posf = XMFLOAT3(0.0f, 1.0f, 0.0f);;
	lightCam = new LookAtCamera(device);
	XMVECTOR pos = lightVec;
	XMVECTOR target = XMVECTOR(XMLoadFloat3(&targetf));
	XMVECTOR up = XMVECTOR(XMLoadFloat3(&upf));
	lightCam->setLookAt(target);
	lightCam->setPos(pos);
	lightCam->setUp(up);
	lightCam->setProjMatrix(XMMatrixPerspectiveFovLH(3.142f * 0.35f, 1, 0.1f, 500.0f));

	//lightCam = new Camera(device, -lightVec, up, lookAt);
	//lightCam->setProjMatrix(&(projMatrix->projMatrix));

	T = XMMATRIX(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	shadowTransform = lightCam->getViewMatrix() * lightCam->getProjMatrix()* T;

	cBufferCPU = (CBufferShadow*)_aligned_malloc(sizeof(CBufferShadow), 16);
	cBufferCPU->shadowTransformMatrix = shadowTransform;


	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));
	cbufferDesc.ByteWidth = sizeof(CBufferShadow);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferCPU;

	device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferGPU);

	//setup light camera Viewport
	shadowViewport.TopLeftX = 0;
	shadowViewport.TopLeftY = 0;
	shadowViewport.Width = static_cast<FLOAT>(shadowMapSize);
	shadowViewport.Height = static_cast<FLOAT>(shadowMapSize);
	shadowViewport.MinDepth = 0.0f;
	shadowViewport.MaxDepth = 1.0f;

	// Setup Shadow Map
	HRESULT hr;
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = shadowMapSize;
	texDesc.Height = shadowMapSize;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL |
		D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	ID3D11Texture2D* ShadowMap = 0;
	hr = device->CreateTexture2D(&texDesc, 0, &ShadowMap);

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = 0;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	hr = device->CreateDepthStencilView(ShadowMap, &dsvDesc, &shadowMapDSV);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	hr = device->CreateShaderResourceView(ShadowMap, &srvDesc, &shadowMapSRV);

	// View saves a reference to the texture so we can release our
	// reference.
	ShadowMap->Release();

	
}

void ShadowMap::update(ID3D11DeviceContext *context) {
	// //Update light Cam cBuffer
	lightCam->update(context);
	shadowTransform = lightCam->getViewMatrix() * lightCam->getProjMatrix()* T;
	// //Update castle cBuffer
	cBufferCPU->shadowTransformMatrix = shadowTransform;
	mapCbuffer(context, cBufferCPU, cBufferGPU, sizeof(CBufferShadow));
	context->VSSetConstantBuffers(6, 1, &cBufferGPU);
}


void ShadowMap::render(ID3D11DeviceContext *context, function<void(ID3D11DeviceContext*)> renderSceneObjects) 
{



		// Render scene to depth buffer from light perspective

		// Bind constant buffers to the relevant slots at each shader stage

		context->PSSetConstantBuffers(6, 1, &cBufferGPU);
		context->VSSetConstantBuffers(6, 1, &cBufferGPU);
		// Set pipeline for Shadow Map
		shadowEffect->bindPipeline(context);

		//Save current Render Target and Depth Stencil to restore when finished
		ID3D11RenderTargetView * tempRT[1] = { 0 };
		ID3D11DepthStencilView *tempDS = nullptr;
		context->OMGetRenderTargets(1, tempRT, &tempDS);
		//Use shadowMap for Depth Stencil Buffer and null for render targets
		ID3D11RenderTargetView* renderTargets[1] = { 0 }; //null render target
		UINT nv=1;
		D3D11_VIEWPORT viewport;
		context->RSGetViewports(&nv, &viewport);
		context->RSSetViewports(1, &shadowViewport);
		context->OMSetRenderTargets(1, renderTargets, shadowMapDSV);
		context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
		lightCam->update(context);
		// Draw scene to shadow map
		//scene->updateScene(context, lightCam);
		renderSceneObjects(context);
		// Update bridge cBuffer


		context->RSSetViewports(1, &viewport);

		// Restore Render Target and  Depth Stencil Buffer
		context->OMSetRenderTargets(1, tempRT, tempDS);




}