/*
 * Original DirectX SDK sample code:
 * Copyright (c) Microsoft Corporation. All rights reserved.
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
 *
 * Modified by Carl Jones (2025) for educational game engine project.
 * Modifications include: [briefly describe your changes]
 */

// Encapsulates D3D device independent, device dependent and window size dependent resources.  For now this supports a single swap chain so there exists a 1:1 correspondance between the System instance and the associated window.
#pragma once
#include <d3d11_2.h>
#include <windows.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui.h>

class System {
	
	// Use DXGI 1.1
	IDXGIFactory1							*dxgiFactory = nullptr;
	IDXGISwapChain							*swapChain = nullptr;

	IDXGIAdapter							*defaultAdapter = nullptr;
	ID3D11Device							*device = nullptr;
	ID3D11DeviceContext						*context = nullptr;
	D3D_FEATURE_LEVEL						supportedFeatureLevel;
	
	ID3D11RenderTargetView					*renderTargetView = nullptr;
	ID3D11DepthStencilView					*depthStencilView = nullptr;
	ID3D11ShaderResourceView				*depthStencilSRV=nullptr;
	ID3D11Texture2D							*depthStencilBuffer = nullptr;

	// Private interface

	// Private constructor
	System(HWND hwnd);

public:
	ImFont* myFont1;
	ImFont* myFont2;
	// Public interface

	// System factory method
	static System* CreateDirectXSystem(HWND hwnd);

	// Destructor
	~System();

	// Setup DirectX interfaces that will be constant - they do not depend on any device
	HRESULT setupDeviceIndependentResources();
	// Setup DirectX interfaces that are dependent upon a specific device
	HRESULT setupDeviceDependentResources(HWND hwnd);
	// Setup window-specific resources including swap chain buffers, texture buffers and resource views that are dependant upon the host window size
	HRESULT setupWindowDependentResources(HWND hwnd);

	// Update methods

	// Resize swap chain buffers according to the given window client area
	HRESULT resizeSwapChainBuffers(HWND hwnd);
	// Present back buffer to the screen
	HRESULT presentBackBuffer();
	void Present() { presentBackBuffer(); };

	// Accessor methods
	ID3D11Device* getDevice();
	ID3D11DeviceContext* getDeviceContext();
	ID3D11Device* GetD3DDevice() const { return device; }
	ID3D11DeviceContext* GetD3DDeviceContext() const { return context; }

	ID3D11RenderTargetView* getBackBufferRTV();
	ID3D11DepthStencilView* getDepthStencil();
	ID3D11ShaderResourceView* getDepthStencilSRV();

	ID3D11RenderTargetView* GetBackBufferRenderTargetView() const { return renderTargetView; }
	ID3D11DepthStencilView* GetDepthStencilView() const { return depthStencilView; }
	ID3D11ShaderResourceView* GetDepthStencilSRV() const { return depthStencilSRV; }

	ID3D11Texture2D* getDepthStencilBuffer();
};
