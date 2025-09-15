#include "stdafx.h"
#include "DynamicCube.h"

DynamicCube::DynamicCube(ID3D11Device* device, ID3D11DeviceContext* context, XMVECTOR pos, int size)
{
	/*-----------------------------------Custom View Port------------------------------------------*/
	cubeViewport.TopLeftX = 0;
	cubeViewport.TopLeftY = 0;
	cubeViewport.Width = size;
	cubeViewport.Height = size;
	cubeViewport.MinDepth = 0.0f;
	cubeViewport.MaxDepth = 1.0f;
	//setup off screen texture

	D3D11_TEXTURE2D_DESC texDesc;
	// fill out texture descrition
	texDesc.Width = cubeViewport.Width;
	texDesc.Height = cubeViewport.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 6;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	//texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;

	//Create Texture
	ID3D11Texture2D* renderTargetTexture = nullptr;
	HRESULT hr = device->CreateTexture2D(&texDesc, 0,
		&renderTargetTexture);
	//Create render target view
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	rtvDesc.Texture2DArray.MipSlice = 0;
	rtvDesc.Texture2DArray.ArraySize = 1;

	/*-----------------------------------Create Dynamic Cube Map for reflection rendering------------------------------------------*/
	for (int i = 0; i < 6; i++)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		hr = device->CreateRenderTargetView(renderTargetTexture, &rtvDesc, &dynamicCubeMapRTV[i]);
	}

	//Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	hr = device->CreateShaderResourceView(renderTargetTexture, &srvDesc, &dynamicCubeMapSRV);
	// View saves reference.
	renderTargetTexture->Release();

	// Create a texture for the depth buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = size;
	depthStencilDesc.Height = size;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;


	// Create a texture for the depth buffer
	ID3D11Texture2D* depthStencilBuffer = nullptr;
	if (SUCCEEDED(hr))
		hr = device->CreateTexture2D(&depthStencilDesc, 0, &depthStencilBuffer);

	// Create a Depth/Stencil view for the depth buffer
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;

	if (SUCCEEDED(hr))
		hr = device->CreateDepthStencilView(depthStencilBuffer, &descDSV, &depthStencilView);
	// View saves reference.
	if (depthStencilBuffer)
		depthStencilBuffer->Release();

	/*-----------------------------------Initialize the cube cameras in the background------------------------------------------*/
	initCubeCameras(device, context, pos);

}
void DynamicCube::initCubeCameras(ID3D11Device* device, ID3D11DeviceContext* context, XMVECTOR pos)
{
	XMVECTOR look, up;
	float x = XMVectorGetX(pos);
	float y = XMVectorGetY(pos);
	float z = XMVectorGetZ(pos);

	look = XMVectorSet(x + 1.0f, y, z, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[0] = new LookAtCamera(device, pos, up, look);
	cubeCameras[0]->setProjMatrix(DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90), 1.0f, 0.5f, 1000.0f));
	cubeCameras[0]->update(context);

	look = XMVectorSet(x - 1.0f, y, z, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[1] = new LookAtCamera(device, pos, up, look);
	cubeCameras[1]->setProjMatrix(DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90), 1.0f, 0.5f, 1000.0f));
	cubeCameras[1]->update(context);

	look = XMVectorSet(x, y + 1.0f, z, 1.0f);
	up = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);

	cubeCameras[2] = new LookAtCamera(device, pos, up, look);
	cubeCameras[2]->setProjMatrix(DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90), 1.0f, 0.5f, 1000.0f));
	cubeCameras[2]->update(context);

	look = XMVectorSet(x, y - 1.0f, z, 1.0f);
	up = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);

	cubeCameras[3] = new LookAtCamera(device, pos, up, look);
	cubeCameras[3]->setProjMatrix(DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90), 1.0f, 0.5f, 1000.0f));
	cubeCameras[3]->update(context);

	look = XMVectorSet(x, y, z + 1.0f, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[4] = new LookAtCamera(device, pos, up, look);
	cubeCameras[4]->setProjMatrix(DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90), 1.0f, 0.5f, 1000.0f));
	cubeCameras[4]->update(context);

	look = XMVectorSet(x, y, z - 1.0f, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[5] = new LookAtCamera(device, pos, up, look);
	cubeCameras[5]->setProjMatrix(DirectX::XMMatrixPerspectiveFovLH(XMConvertToRadians(90), 1.0f, 0.5f, 1000.0f));
	cubeCameras[5]->update(context);
}
DynamicCube::~DynamicCube()
{

}
void DynamicCube::updateCubeCameras(ID3D11DeviceContext* context, XMVECTOR pos)
{
	XMVECTOR look, up;
	float x = XMVectorGetX(pos);
	float y = XMVectorGetY(pos);
	float z = XMVectorGetZ(pos);

	look = XMVectorSet(x + 1.0f, y, z, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[0]->setPos(pos);
	cubeCameras[0]->setLookAt(look);
	cubeCameras[0]->setUp(up);
	cubeCameras[0]->update(context);

	look = XMVectorSet(x - 1.0f, y, z, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[1]->setPos(pos);
	cubeCameras[1]->setLookAt(look);
	cubeCameras[1]->setUp(up);
	cubeCameras[1]->update(context);

	look = XMVectorSet(x, y + 1.0f, z, 1.0f);
	up = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);

	cubeCameras[2]->setPos(pos);
	cubeCameras[2]->setLookAt(look);
	cubeCameras[2]->setUp(up);
	cubeCameras[2]->update(context);

	look = XMVectorSet(x, y - 1.0f, z, 1.0f);
	up = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);

	cubeCameras[3]->setPos(pos);
	cubeCameras[3]->setLookAt(look);
	cubeCameras[3]->setUp(up);
	cubeCameras[3]->update(context);

	look = XMVectorSet(x, y, z + 1.0f, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[4]->setPos(pos);
	cubeCameras[4]->setLookAt(look);
	cubeCameras[4]->setUp(up);
	cubeCameras[4]->update(context);

	look = XMVectorSet(x, y, z - 1.0f, 1.0f);
	up = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);

	cubeCameras[5]->setPos(pos);
	cubeCameras[5]->setLookAt(look);
	cubeCameras[5]->setUp(up);
	cubeCameras[5]->update(context);
}
void DynamicCube::render(System* system, function<void(ID3D11DeviceContext*)> renderSceneObjects)
{
	ID3D11DeviceContext* context = system->getDeviceContext();

	//Save current viewport
	UINT nv = 1;
	D3D11_VIEWPORT viewport;
	context->RSGetViewports(&nv, &viewport);
	//Set Viewport
	context->RSSetViewports(1, &cubeViewport);
	//Save current Render Target and Depth Stencil to restore when finished
	//[1] = { 0 };
	//ID3D11DepthStencilView* tempDS = nullptr;
	//context->OMGetRenderTargets(1, tempRT, &tempDS);
	//unbind 	dynamicCubeSRV
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	context->PSSetShaderResources(6, 1, nullSRV);
	ID3D11RenderTargetView* renderTarget[1];
	// Clear new render target and original depth stencil
	static const FLOAT clearColorOS[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	static const FLOAT clearColorBlue[4] = { 0.0f,0.0f,1.0f,0.0f };

	for (int i = 0; i < 6; i++)
	{
		cubeCameras[i]->update(context);
		renderTarget[0] = dynamicCubeMapRTV[i];
		context->OMSetRenderTargets(1, renderTarget, depthStencilView);

		if (i == 0)
			context->ClearRenderTargetView(dynamicCubeMapRTV[i], clearColorBlue);
		else
			context->ClearRenderTargetView(dynamicCubeMapRTV[i], clearColorOS);
		context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		renderSceneObjects(context);
	}

	//mainCamera->update(context);

	//Set Original Viewport
	context->RSSetViewports(1, &viewport);
	// Restore Render Target and  Depth Stencil Buffer
	ID3D11RenderTargetView* tempRT= system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &tempRT, system->getDepthStencil());
	//Bind dynamicCubeMapSRV to pixel shader slot 7
	context->PSSetShaderResources(6, 1, &dynamicCubeMapSRV);

}
