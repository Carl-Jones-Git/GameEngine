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

#pragma once

#include <DirectXMath.h>
#include <Effect.h>
#include <LookAtCamera.h>
#include <CBufferStructures.h>

#ifndef PC_BUILD
#include "..\Common\DeviceResources.h"
#else
#include <System.h>
#endif
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

	#ifdef PC_BUILD
	void render(System* system, function<void(ID3D11DeviceContext*)> renderSceneObjects);
#else
	void render(std::shared_ptr<DX::DeviceResources> system, function<void(ID3D11DeviceContext*)> renderSceneObjects);
#endif

	void setShadowMatrix(ID3D11DeviceContext *context) { context->VSSetConstantBuffers(5, 1, &cBufferGPU); }
	XMMATRIX  getShadowMatrix(){ return shadowTransform; };
	ID3D11ShaderResourceView *getSRV(){ return shadowMapSRV; };

	~ShadowMap();
};

