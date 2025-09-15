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
#include "Texture.h"
#include <iostream>
#include <exception>

Texture::Texture(ID3D11DeviceContext* context, ID3D11Device *device, const std::wstring& filename)
{
	SRV = nullptr;
	ID3D11Resource *resource = static_cast<ID3D11Resource*>(texture);
	HRESULT hr;
	// Get filename extension
	wstring ext = filename.substr(filename.length() - 4);

	try
	{
		if (0 == ext.compare(L".bmp") || 0 == ext.compare(L".jpg") || 0 == ext.compare(L".png") || 0 == ext.compare(L".tif"))
			hr = DirectX::CreateWICTextureFromFileEx(device, context, filename.c_str(), 4096, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, DirectX::WIC_LOADER_FLAGS::WIC_LOADER_DEFAULT, &resource, &SRV);
		else if (0 == ext.compare(L".dds"))
			hr = DirectX::CreateDDSTextureFromFileEx(device,context, filename.c_str(),4096, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,0,0,false, &resource, &SRV);
		else throw exception("Texture file format not supported");
	}
	catch (exception& e)
	{
		cout << "Texture was not loaded:\n";
		cout << e.what() << endl;
	}

	texture = static_cast<ID3D11Texture2D*>(resource);
}
Texture::Texture( ID3D11Device* device, const std::wstring& filename)
{
	SRV = nullptr;
	ID3D11Resource* resource = static_cast<ID3D11Resource*>(texture);
	HRESULT hr;
	// Get filename extension
	wstring ext = filename.substr(filename.length() - 4);

	try
	{
		if (0 == ext.compare(L".bmp") || 0 == ext.compare(L".jpg") || 0 == ext.compare(L".png") || 0 == ext.compare(L".tif"))
			hr = DirectX::CreateWICTextureFromFile(device, filename.c_str(), &resource, &SRV);
		else if (0 == ext.compare(L".dds"))
			hr = DirectX::CreateDDSTextureFromFile(device,  filename.c_str(), &resource, &SRV);
		else throw exception("Texture file format not supported");
	}
	catch (exception& e)
	{
		cout << "Texture was not loaded:\n";
		cout << e.what() << endl;
	}

	texture = static_cast<ID3D11Texture2D*>(resource);
}

Texture::~Texture()
{
	cout << "Destroying texture" << endl;
	if (texture != nullptr)
		texture->Release();
	if (SRV != nullptr)
		SRV->Release();
	if (DSV != nullptr)
		DSV->Release();
	if (RTV != nullptr)
		RTV->Release();
}
