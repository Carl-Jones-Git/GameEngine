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
#include<LookAtCamera.h>
#include <System.h>

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

