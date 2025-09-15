/*
 * Copyright (c) 2023 Carl Jones
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <Includes.h>
#include "Flare.h"

void Flare::updateTextSize(ID3D11DeviceContext* context, int width, int height) {
	cBufferTextSizeCPU->Width = width;
	cBufferTextSizeCPU->Height = height;
	mapCbuffer(context, cBufferTextSizeCPU, cBufferTextSizeGPU, sizeof(CBufferTextSize));
	context->PSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferTextSizeGPU);
}
void Flare::initCBuffer(ID3D11Device* device, int _texWidth, int _texHeight) {

	// Allocate 16 byte aligned block of memory for "main memory" copy of CBufferTextSize
	cBufferTextSizeCPU = (CBufferTextSize*)_aligned_malloc(sizeof(CBufferTextSize), 16);

	// Fill out cBufferTextSizeCPU
	cBufferTextSizeCPU->Width = _texWidth;
	cBufferTextSizeCPU->Height = _texHeight;
	cBufferTextSizeCPU->GaussWidth = 1.5;

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
HRESULT Flare::init(ID3D11Device *device, XMFLOAT3 position)
{
	XMCOLOR colour(instances[0].materials[0]->getDiffuse().x, instances[0].materials[0]->getDiffuse().y, instances[0].materials[0]->getDiffuse().z, instances[0].materials[0]->getDiffuse().w); 
	FlareVertexStruct vertices[] = {
		{ position, XMFLOAT3(-1.0f, -1.0f, 0.0f), colour},
		{ position, XMFLOAT3(-1.0f, 1.0f, 0.0f), colour },
		{ position, XMFLOAT3(1.0f, -1.0f, 0.0f), colour },
		{ position, XMFLOAT3(1.0f, 1.0f, 0.0f), colour }
	};

	// Setup flare vertex buffer
	D3D11_BUFFER_DESC vertexDesc;
	D3D11_SUBRESOURCE_DATA vertexdata;

	ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&vertexdata, sizeof(D3D11_SUBRESOURCE_DATA));

	vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexDesc.ByteWidth = sizeof(FlareVertexStruct )*4;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexdata.pSysMem = vertices;

	HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexdata, &vertexBuffer);
	initCBuffer(device);
	return S_OK;
}

Flare::~Flare()
{

}
#ifdef PC_BUILD
void Flare::render(System* system)
{
	// Draw the lense flares (Draw all transparent objects last)
	ID3D11DeviceContext* context = system->getDeviceContext();

	D3D11_VIEWPORT 		currentVP;
	UINT nCurrentVP = 1;
	context->RSGetViewports(&nCurrentVP, &currentVP);
	updateTextSize(context, currentVP.Width, currentVP.Height);
	// Set NULL depth buffer so we can also use the Depth Buffer as a shader
	// resource this is OK as we don’t need depth testing for the flares
	ID3D11RenderTargetView* tempRT = system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &tempRT, NULL);
	ID3D11ShaderResourceView* depthSRV = system->getDepthStencilSRV();
	context->VSSetShaderResources(5, 1, &depthSRV);
	renderInstances(context);
	// Use Null SRV to release depth shader resource so it is available for writing
	ID3D11ShaderResourceView* nullSRV = NULL;
	context->VSSetShaderResources(5, 1, &nullSRV);
	// Return default (read and write) depth buffer view.
	tempRT = system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &tempRT, system->getDepthStencil());

}
#else
void Flare::render(std::shared_ptr<DX::DeviceResources> system)
{
	// Draw the lense flares (Draw all transparent objects last)
	ID3D11DeviceContext* context = system->GetD3DDeviceContext();


	D3D11_VIEWPORT 		currentVP;
	UINT nCurrentVP = 1;
	context->RSGetViewports(&nCurrentVP, &currentVP);
	updateTextSize(context, currentVP.Width, currentVP.Height);
	// Set NULL depth buffer so we can also use the Depth Buffer as a shader
	// resource this is OK as we don’t need depth testing for the flares
	ID3D11RenderTargetView* tempRT = system->GetBackBufferRenderTargetView();
	context->OMSetRenderTargets(1, &tempRT, NULL);
	ID3D11ShaderResourceView* depthSRV = system->GetDepthStencilSRV();
	context->VSSetShaderResources(5, 1, &depthSRV);
	renderInstances(context);
	// Use Null SRV to release depth shader resource so it is available for writing
	ID3D11ShaderResourceView* nullSRV = NULL;
	context->VSSetShaderResources(5, 1, &nullSRV);
	// Return default (read and write) depth buffer view.
	tempRT = system->GetBackBufferRenderTargetView();
	context->OMSetRenderTargets(1, &tempRT, system->GetDepthStencilView());

}
#endif


void Flare::render(ID3D11DeviceContext *context, int instanceIndex )
{
	D3D11_VIEWPORT 		currentVP;
	UINT nCurrentVP = 1;
	context->RSGetViewports(&nCurrentVP, &currentVP);
	updateTextSize(context, currentVP.Width, currentVP.Height);

	// Validate object before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !effect)
		return;

	effect->bindPipeline(context);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (instances[instanceIndex].materials.size() > 0)
		instances[instanceIndex].materials[0]->update(context);
	if (sampler) 
		context->PSSetSamplers(0, 1, &sampler);

	// Set vertex layout
	context->IASetInputLayout(effect->getVSInputLayout());

	// Set vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(FlareVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	context->Draw(4, 0);
}
