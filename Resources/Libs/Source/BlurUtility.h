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
#include <CBufferStructures.h>
#include "Model.h"
#include "System.h"

class Quad;
class Effect;
class BlurUtility
{
	D3D11_VIEWPORT							offScreenViewport;
	ID3D11ShaderResourceView				*intermedHSRV = nullptr;
	ID3D11RenderTargetView					*intermedHRTV = nullptr;
	ID3D11ShaderResourceView				*intermedVSRV = nullptr;
	ID3D11RenderTargetView					*intermedVRTV = nullptr;
	ID3D11ShaderResourceView				*intermedSRV = nullptr;
	ID3D11RenderTargetView					*intermedRTV = nullptr;
	ID3D11ShaderResourceView				*depthBlurSRV = nullptr;
	ID3D11DepthStencilView					*depthStencilViewBlur = nullptr;
	ID3D11DeviceContext						*context = nullptr;
	ID3D11Device							*device = nullptr;
	Quad									*screenQuad = nullptr;

	ID3D11BlendState						*alphaOnBlendState = nullptr;
	ID3D11VertexShader						*screenQuadVS = nullptr;
	ID3D11PixelShader						*emissivePS = nullptr;
	ID3D11PixelShader						*horizontalBlurPS = nullptr;
	ID3D11PixelShader						*verticalBlurPS = nullptr;
	ID3D11PixelShader						*textureCopyPS = nullptr;
	ID3D11PixelShader						*depthCopyPS = nullptr;
	ID3D11VertexShader						*perPixelLightingVS = nullptr;

	Effect									*defaultEffect = nullptr;
	CBufferTextSize * cBufferTextSizeCPU = nullptr;
	ID3D11Buffer *cBufferTextSizeGPU = nullptr;
	ID3D11SamplerState			*sampler = nullptr;
	void initCBuffer(ID3D11Device *device, int _blurWidth, int _blurHeight);
	void updateTextSize(ID3D11DeviceContext *context, int width, int height);
public:
	BlurUtility(ID3D11Device *deviceIn, ID3D11DeviceContext *contextIn, int blurWidth = 512, int blurHeight = 512);
	HRESULT setupBlurRenderTargets(ID3D11Device *deviceIn, int _blurWidth, int _blurHeight);
	void blurModel(System* system, Model*orb );
	void blurModels(System* system, function<void(ID3D11DeviceContext*)> renderGlowObjects);

	~BlurUtility();
};

