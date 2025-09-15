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
#include <d3d11_2.h>
#include <DirectXMath.h>

class Texture;
class Material;
class Effect;

class Mesh
{
protected:
	Material *material = nullptr;
	Effect *effect = nullptr;
	UINT numVert=0;
	UINT numInd = 0;
	ID3D11Buffer				*vertexBuffer = nullptr;
	ID3D11Buffer				*indexBuffer = nullptr;
	// Augment grid with texture view
	ID3D11ShaderResourceView		*textureResourceView = nullptr;
	ID3D11SamplerState				*linearSampler = nullptr;

public:
	Mesh(ID3D11Device *device, Effect *_effect, ID3D11ShaderResourceView *tex_view, Material *_material);
	void render(ID3D11DeviceContext *context);
	~Mesh();
};

