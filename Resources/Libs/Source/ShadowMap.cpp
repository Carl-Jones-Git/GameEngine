#include <Includes.h>
#include <ShadowMap.h>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

ShadowMap::ShadowMap(ID3D11Device *device,  XMVECTOR lightVec, int _shadowMapSize)
{
	shadowMapSize = _shadowMapSize;

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
	//lightCam->setProjMatrix(XMMatrixPerspectiveFovLH(3.142f * 0.35f, 1, 200.0f, 500.0f));
	lightCam->setProjMatrix(XMMatrixOrthographicLH(200, 100, 50.0f, 250.0f));

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
	D3D11_SAMPLER_DESC samplerDesc;

	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	// Return 0 for points outside the light frustum 										  
	// to put them in shadow. AddressU = BORDER;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samplerDesc.BorderColor[0] = 0.0f;// { 0.0f, 0.0f, 0.0f, 0.0f };
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

	device->CreateSamplerState(&samplerDesc, &shadowSampler);
	initShadowMap(device,shadowMapSize);
	
}
ShadowMap::~ShadowMap()
{
	cout << "Menu Destructor called" << endl;
	if (shadowMapSRV)
		shadowMapSRV->Release();
	if (shadowMapDSV)
		shadowMapDSV->Release();	
	if (shadowSampler)
		shadowSampler->Release();

	delete(lightCam);

	if (cBufferCPU)
		_aligned_free(cBufferCPU);
	if (cBufferGPU)
		cBufferGPU->Release();

}

ID3D11ShaderResourceView* ShadowMap::setMapSize(ID3D11Device* device, int newMapSize)
{
	shadowMapSize = newMapSize;
	if (shadowMapSRV != nullptr)
		shadowMapSRV->Release();
	if (shadowMapDSV != nullptr)
		shadowMapDSV->Release();
	return initShadowMap(device, newMapSize);
}

ID3D11ShaderResourceView* ShadowMap::initShadowMap(ID3D11Device* device, int shadowMapSize)
{
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
	return shadowMapSRV;
}

void ShadowMap::update(ID3D11DeviceContext *context) {
	// //Update light Cam cBuffer
	lightCam->update(context);
	shadowTransform = lightCam->getViewMatrix() * lightCam->getProjMatrix()* T;
	// //Update shadow cBuffer
	cBufferCPU->shadowTransformMatrix = shadowTransform;
	mapCbuffer(context, cBufferCPU, cBufferGPU, sizeof(CBufferShadow));
	context->VSSetConstantBuffers(5, 1, &cBufferGPU);
}


#ifdef PC_BUILD


void ShadowMap::render(System *system, function<void(ID3D11DeviceContext*)> renderSceneObjects) 
{
	ID3D11DeviceContext* context = system->getDeviceContext();
	// Render scene to depth buffer from light perspective
	// Bind constant buffers to the relevant slots at each shader stage
	context->PSSetConstantBuffers(5, 1, &cBufferGPU);
	//unbind shadowMapSRV
	ID3D11ShaderResourceView* nullSRV =nullptr ;
	context->PSSetShaderResources(7, 1, &nullSRV);

	//Use shadowMap for Depth Stencil Buffer and null for render targets
	ID3D11RenderTargetView* renderTarget= nullptr; //null render target
	UINT nv=1;
	D3D11_VIEWPORT viewport;
	context->RSGetViewports(&nv, &viewport);
	context->RSSetViewports(1, &shadowViewport);
	context->OMSetRenderTargets(1, &renderTarget, shadowMapDSV);
	context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
	lightCam->update(context);

	// Draw scene to shadow map
	renderSceneObjects(context);

	//Restore Viewport
	context->RSSetViewports(1, &viewport);
	// Restore Render Target and  Depth Stencil Buffer
	ID3D11RenderTargetView* tempRT = system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &tempRT, system->getDepthStencil());
	//Bind shadowMapSRV to pixel shader slot 7
	context->PSSetShaderResources(7, 1, &shadowMapSRV);
	context->PSSetSamplers(7, 1, &shadowSampler);

}

#else

void ShadowMap::render(std::shared_ptr<DX::DeviceResources> system, function<void(ID3D11DeviceContext*)> renderSceneObjects)
{
	ID3D11DeviceContext* context = system->GetD3DDeviceContext();
	// Render scene to depth buffer from light perspective
	// Bind constant buffers to the relevant slots at each shader stage
	context->PSSetConstantBuffers(5, 1, &cBufferGPU);
	//unbind shadowMapSRV
	ID3D11ShaderResourceView* nullSRV = nullptr;
	context->PSSetShaderResources(7, 1, &nullSRV);

	//Use shadowMap for Depth Stencil Buffer and null for render targets
	ID3D11RenderTargetView* renderTarget = nullptr; //null render target
	UINT nv = 1;
	D3D11_VIEWPORT viewport;
	context->RSGetViewports(&nv, &viewport);
	context->RSSetViewports(1, &shadowViewport);
	context->OMSetRenderTargets(1, &renderTarget, shadowMapDSV);
	context->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0);
	lightCam->update(context);

	// Draw scene to shadow map
	renderSceneObjects(context);

	//Restore Viewport
	context->RSSetViewports(1, &viewport);
	// Restore Render Target and  Depth Stencil Buffer
	ID3D11RenderTargetView* tempRT = system->GetBackBufferRenderTargetView();
	context->OMSetRenderTargets(1, &tempRT, system->GetDepthStencilView());
	//Bind shadowMapSRV to pixel shader slot 7
	context->PSSetShaderResources(7, 1, &shadowMapSRV);
	context->PSSetSamplers(7, 1, &shadowSampler);

}

#endif