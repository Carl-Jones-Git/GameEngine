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
#include <string>
#include <vector>
#include <cstdint>
#include <d3d11_2.h>

class Texture
{
	// Direct3D scene textures and resource views
	ID3D11Texture2D							*texture = nullptr;
	ID3D11ShaderResourceView				*SRV = nullptr;
	ID3D11DepthStencilView					*DSV = nullptr;
	ID3D11RenderTargetView					*RTV = nullptr;
public:

	Texture(ID3D11Device *device, const std::wstring& filename);
	Texture(ID3D11DeviceContext* context,ID3D11Device* device, const std::wstring& filename);
	ID3D11ShaderResourceView *getShaderResourceView(){ return SRV; };
	ID3D11Texture2D* getTexture() { return texture; };
	~Texture();
};

