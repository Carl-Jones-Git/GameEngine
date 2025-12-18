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

#include "Includes.h"
#include <BaseModel.h>

BaseModel::BaseModel(ID3D11Device *device, shared_ptr<Effect> _effect, std::shared_ptr<Material> _material, int _meshNumber) {
	meshNumber = _meshNumber;
	effect = _effect;
	Instance instance= Instance(XMMatrixIdentity(), _material);
	instances.push_back(instance);
	createDefaultLinearSampler(device);
	initCBuffer(device);
}

void BaseModel::initCBuffer(ID3D11Device *device){	
	
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferBasic
	cBufferModelCPU = (CBufferModel*)_aligned_malloc(sizeof(CBufferModel), 16);

	// Fill out cBufferModelCPU
	cBufferModelCPU->worldMatrix = XMMatrixIdentity();
	cBufferModelCPU->worldITMatrix = XMMatrixIdentity();

	// Create GPU resource memory copy of cBufferBasic
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferModel);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferModelCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,&cBufferModelGPU);
}

void BaseModel::setWorldMatrix(XMMATRIX _worldMatrix,int n){
	instances[n].worldMatrix = _worldMatrix;
	cBufferModelCPU->worldMatrix = _worldMatrix;
	XMVECTOR det=XMMatrixDeterminant(_worldMatrix);
	cBufferModelCPU->worldITMatrix = XMMatrixInverse(&det, XMMatrixTranspose(_worldMatrix));
}

void BaseModel::update(ID3D11DeviceContext* context,int i) {
	cBufferModelCPU->worldMatrix = instances[i].worldMatrix;
	XMVECTOR det = XMMatrixDeterminant(instances[i].worldMatrix);
	cBufferModelCPU->worldITMatrix = XMMatrixInverse(&det, XMMatrixTranspose(instances[i].worldMatrix));
	mapCbuffer(context, cBufferModelCPU, cBufferModelGPU, sizeof(CBufferModel));
	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);
}

void BaseModel::createDefaultLinearSampler(ID3D11Device *device){
	
	// If textures are used a sampler is required for the pixel shader to sample the texture
	D3D11_SAMPLER_DESC linearDesc;
	ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
	linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.MinLOD = 0.0f;
	linearDesc.MaxLOD = D3D11_FLOAT32_MAX;
	linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	device->CreateSamplerState(&linearDesc, &sampler);
}

void BaseModel::setMaterial(ID3D11Device* device, std::shared_ptr<Material>_material, int instanceIndex, int materialIndex) {
	instances[instanceIndex].materials[materialIndex] = _material;
}

BaseModel::~BaseModel() {
	cout << "BaseModel Destructor Called" << endl;
	if (vertexBuffer)
		vertexBuffer->Release();
	if (indexBuffer)
		indexBuffer->Release();
	if (sampler)
		sampler->Release();

	if (cBufferModelCPU)
		_aligned_free(cBufferModelCPU);
	if (cBufferModelGPU)
		cBufferModelGPU->Release();
	cout << "BaseModel Destructor Complete" << endl;
}
