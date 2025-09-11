#include "Includes.h"
#include "SkinUtility.h"
#include "Model.h"
#include <Quad.h>
#include <Effect.h>
#include <VertexStructures.h>


SkinUtility::SkinUtility(ID3D11Device* deviceIn, ID3D11DeviceContext* contextIn, int _blurWidth, int _blurHeight, float _weight)
{
	device = deviceIn;
	context = contextIn;
	weight = _weight;
	initCBuffer(device, _blurWidth, _blurHeight,1.0);
	setupBlurRenderTargets(deviceIn, _blurWidth, _blurHeight);
	char* tmpShaderBytecode = nullptr;

	ID3D11InputLayout* screenQuadVSInputLayout = nullptr;
	SIZE_T shaderBytes = LoadShader("Shaders\\cso\\screen_quad_vs.cso", &tmpShaderBytecode);
	device->CreateVertexShader(tmpShaderBytecode, shaderBytes, NULL, &screenQuadVS);
	device->CreateInputLayout(basicVertexDesc, ARRAYSIZE(basicVertexDesc), tmpShaderBytecode, shaderBytes, &screenQuadVSInputLayout);
	free(tmpShaderBytecode);
	screenQuad = new Quad(device, screenQuadVSInputLayout);
	shaderBytes = LoadShader("Shaders\\cso\\per_pixel_lighting_vs.cso", &tmpShaderBytecode);
	device->CreateVertexShader(tmpShaderBytecode, shaderBytes, NULL, &perPixelLightingVS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\convolve_u_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &horizontalBlurPS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\convolve_v_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &verticalBlurPS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\basic_skin_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &skinPS);
	free(tmpShaderBytecode);
	shaderBytes = LoadShader("Shaders\\cso\\copy_ps.cso", &tmpShaderBytecode);
	device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &textureCopyPS);
	free(tmpShaderBytecode);
	//shaderBytes = LoadShader("Shaders\\cso\\copy_depth_ps.cso", &tmpShaderBytecode);
	//device->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &depthCopyPS);
	//free(tmpShaderBytecode);

	defaultEffect = make_shared< Effect>(device, "Shaders\\cso\\basic_colour_vs.cso", "Shaders\\cso\\basic_colour_ps.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	texSpaceEffect = make_shared< Effect>(device, "Shaders\\cso\\texture_space_vs.cso", "Shaders\\cso\\skin_rad_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	texSpaceEffect->setCullMode(device, D3D11_CULL_NONE);
	skinEffect = make_shared< Effect>(device, "Shaders\\cso\\per_pixel_lighting_vs.cso", "Shaders\\cso\\skin_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	D3D11_BLEND_DESC blendDesc;
	defaultEffect->getBlendState()->GetDesc(&blendDesc);//the effect is initialised with the default blend state
	blendDesc.AlphaToCoverageEnable = FALSE; // Use pixel coverage info from rasteriser (default)
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	device->CreateBlendState(&blendDesc, &alphaOnBlendState);
}
void SkinUtility::initCBuffer(ID3D11Device* device, int _blurWidth, int _blurHeight,float _GaussWidth) {

	// Allocate 16 byte aligned block of memory for "main memory" copy of CBufferTextSize
	cBufferTextSizeCPU = (CBufferTextSize*)_aligned_malloc(sizeof(CBufferTextSize), 16);

	// Fill out cBufferTextSizeCPU
	cBufferTextSizeCPU->Width = _blurWidth;
	cBufferTextSizeCPU->Height = _blurHeight;
	cBufferTextSizeCPU->GaussWidth = _GaussWidth;
	// Create GPU resource memory copy of CBufferTextSize
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferTextSize);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferTextSizeCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,
		&cBufferTextSizeGPU);
}

void SkinUtility::updateTextSize(ID3D11DeviceContext* context) {
	mapCbuffer(context, cBufferTextSizeCPU, cBufferTextSizeGPU, sizeof(CBufferTextSize));
	context->PSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
}
void SkinUtility::blurModel(Model* obj, ID3D11ShaderResourceView* depthSRV)
{
	// Blur the object 
	if (obj) {

		// Ensure default states
		FLOAT			blendFactor[] = { 1, 1, 1, 1 };
		context->OMSetDepthStencilState(defaultEffect->getDepthStencilState(), 0);
		context->OMSetBlendState(defaultEffect->getBlendState(), blendFactor, 0xFFFFFFFF);

		// Store Scene render target and depth buffer to restore when finished
		ID3D11RenderTargetView* tempRT[1] = { 0 };
		ID3D11DepthStencilView* tempDS = nullptr;
		context->OMGetRenderTargets(1, tempRT, &tempDS);

		// Clear off screen render targets

		FLOAT clearColorBlur[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		context->ClearRenderTargetView(intermedRTV, clearColorBlur);
		context->ClearRenderTargetView(intermedHRTV, clearColorBlur);
		for (int i = 0; i < 6; i++)
		{
			context->ClearRenderTargetView(irradTexRTV[i], clearColorBlur);
		}
		context->ClearDepthStencilView(depthStencilViewBlur, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// Store Scene viewport to put them back when finished
		D3D11_VIEWPORT 		currentVP;
		UINT nCurrentVP = 1;
		context->RSGetViewports(&nCurrentVP, &currentVP);
		// Set Viewport Offscreen 
		context->RSSetViewports(1, &offScreenViewport);

		//Render object to be blured offscreen
		context->OMSetRenderTargets(1, &intermedRTV, nullptr);

		shared_ptr<Effect>  tmp = obj->getEffect();
		obj->setEffect(texSpaceEffect);
		obj->update(context);
		obj->render(context);

		cBufferTextSizeCPU->Width = offScreenViewport.Width*0.5;
		cBufferTextSizeCPU->Height = offScreenViewport.Height;
		updateTextSize(context);
		weight = 8;
		//FLOAT gaussWidth[6] = { 0.042 * weight, 0.220 * weight, 0.433 * weight, 0.753 * weight, 1.412 * weight, 2.722 * weight };
		FLOAT gaussWidth[6] = { 0.0064 * weight,0.0484 * weight,0.187 * weight,0.567 * weight,1.99 * weight,7.41 * weight };
		bool seriesFilter =true;
		//loopx6
		for (int i = 0; i < 6; i++)
		{
			cBufferTextSizeCPU->GaussWidth = gaussWidth[i];
			updateTextSize(context);
			context->PSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
			//Horizontal blur
			context->OMSetRenderTargets(1, &intermedHRTV, nullptr);
			
			if(!seriesFilter|| i == 0)
				context->PSSetShaderResources(0, 1, &intermedSRV);
			else
				context->PSSetShaderResources(0, 1, &irradTexSRV[i-1]);

			context->VSSetShader(screenQuadVS, 0, 0);
			context->PSSetShader(horizontalBlurPS, 0, 0);
			//context->PSSetShader(textureCopyPS, 0, 0);
			screenQuad->render(context);
			//Vertical blur
			context->OMSetRenderTargets(1, &irradTexRTV[i], nullptr);
			context->PSSetShaderResources(0, 1, &intermedHSRV);
			context->VSSetShader(screenQuadVS, 0, 0);
			context->PSSetShader(verticalBlurPS, 0, 0);
			//context->PSSetShader(textureCopyPS, 0, 0);
			screenQuad->render(context);
		}
		// Restore Scene rendertarget dont need depth buffer here 
		context->OMSetRenderTargets(1, tempRT, tempDS);

		// Restore Scene viewport 
		context->RSSetViewports(1, &currentVP);

		ID3D11ShaderResourceView* TSDtex[] = {irradTexSRV[0],irradTexSRV[1],irradTexSRV[2],irradTexSRV[3],irradTexSRV[4],irradTexSRV[5],obj->getMaterial()->getTexture(1),obj->getMaterial()->getTexture(0)};

		//Save obj textures to put back later
		ID3D11ShaderResourceView* tex[] = { obj->getMaterial()->getTexture(0),obj->getMaterial()->getTexture(1),obj->getMaterial()->getTexture(2) };

		//Render using TSD
		obj->setEffect(skinEffect);
		obj->getMaterial()->setTextures(TSDtex, 8);
		obj->render(context);

		//Restore Textures and effect
		obj->setEffect(tmp);
		obj->getMaterial()->setTextures(tex,3);;

		// Create "nullSRV"  to release intermedVSRV shader resource view
		ID3D11ShaderResourceView* nullSRV[1] = { 0 };
		context->PSSetShaderResources(0, 1, nullSRV);
		// Restore Scene depth buffer.
		context->OMSetRenderTargets(1, tempRT, tempDS);
		
		tempRT[0]->Release();
		tempDS->Release();
	}
}




HRESULT SkinUtility::setupBlurRenderTargets(ID3D11Device* deviceIn, int blurWidth, int blurHeight) {

	if (!device || !context)
		return E_FAIL;

	cBufferTextSizeCPU->Width = blurWidth;
	cBufferTextSizeCPU->Height = blurHeight;

	updateTextSize(context);

	// Release as we might be resizing
	if (intermedSRV)
		intermedSRV->Release();
	if (intermedRTV)
		intermedRTV->Release();
	if (intermedHSRV)
		intermedHSRV->Release();
	if (intermedHRTV)
		intermedHRTV->Release();


	// Setup Render Targets

	HRESULT hr;

	// Setup viewport for offscreen rendering
	offScreenViewport.TopLeftX = 0;
	offScreenViewport.TopLeftY = 0;
	offScreenViewport.Width = blurWidth;
	offScreenViewport.Height = blurHeight;
	offScreenViewport.MinDepth = 0.0f;
	offScreenViewport.MaxDepth = 1.0f;

	// Setup texture description for offscreen rendering
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = blurWidth;
	texDesc.Height = blurHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	ID3D11Texture2D* tex = 0;

	// Setup shader resource view description for offscreen rendering.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	viewDesc.Texture2D.MipLevels = 1;
	viewDesc.Texture2D.MostDetailedMip = 0;
	// Setup render targetview  description for offscreen rendering.
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = texDesc.Format;
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;

	//Create offscreeen Textures, ShaderResourceViews and RenderTargetViews
	hr = device->CreateTexture2D(&texDesc, 0, &tex);
	hr = device->CreateShaderResourceView(tex, &viewDesc, &intermedHSRV);
	hr = device->CreateRenderTargetView(tex, &rtvDesc, &intermedHRTV);
	// Cleanup--we only need the views.
	tex->Release();

	for (int i = 0; i < 6; i++)
	{
		hr = device->CreateTexture2D(&texDesc, 0, &tex);
		hr = device->CreateShaderResourceView(tex, &viewDesc, &irradTexSRV[i]);
		hr = device->CreateRenderTargetView(tex, &rtvDesc, &irradTexRTV[i]);
		// Cleanup--we only need the views.
		tex->Release();
	}
	hr = device->CreateTexture2D(&texDesc, 0, &tex);
	hr = device->CreateShaderResourceView(tex, &viewDesc, &intermedSRV);
	hr = device->CreateRenderTargetView(tex, &rtvDesc, &intermedRTV);
	// Cleanup--we only need the views.
	tex->Release();

}


SkinUtility::~SkinUtility()
{
	if (cBufferTextSizeCPU)
		_aligned_free(cBufferTextSizeCPU);

	if (cBufferTextSizeGPU)
		cBufferTextSizeGPU->Release();

	if (intermedHSRV)
		intermedHSRV->Release();
	if (intermedHRTV)
		intermedHRTV->Release();
	for (int i = 0; i < 6; i++)
	{
		if (irradTexSRV[i])
			irradTexSRV[i]->Release();
		if (irradTexRTV[i])
			irradTexRTV[i]->Release();
	
	}
	if (intermedSRV)
		intermedSRV->Release();
	if (intermedRTV)
		intermedRTV->Release();


	if (depthStencilViewBlur)
		depthStencilViewBlur->Release();
	if (alphaOnBlendState)
		alphaOnBlendState->Release();
	if (screenQuadVS)
		screenQuadVS->Release();

	if (horizontalBlurPS)
		horizontalBlurPS->Release();
	if (verticalBlurPS)
		verticalBlurPS->Release();
	if (textureCopyPS)
		textureCopyPS->Release();

	if (perPixelLightingVS)
		perPixelLightingVS->Release();

	if (screenQuad)
		delete(screenQuad);

}
