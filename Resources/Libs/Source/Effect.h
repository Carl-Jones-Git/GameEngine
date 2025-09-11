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
#include <Utils.h>
#include <iostream>
#include <stdexcept>
#include <memory>

/**
 * Helper to safely release DirectX resources
 */
template <class T>
inline void SafeRelease(T*& resource) {
    if (resource) {
        resource->Release();
        resource = nullptr;
    }
}

/**
 * Effect class - manages DirectX rendering pipeline states and shaders
 *
 * This class handles the creation, configuration, and binding of DirectX rendering
 * pipeline components including shaders, render states, and input layouts.
 */
class Effect {
private:
    // Pipeline Input Layout
    ID3D11InputLayout* VSInputLayout = nullptr;

    // Pipeline Shaders
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11GeometryShader* GeometryShader = nullptr;
    ID3D11HullShader* HullShader = nullptr;
    ID3D11DomainShader* DomainShader = nullptr;

    // Pipeline States
    ID3D11RasterizerState* RasterizerState = nullptr;
    ID3D11DepthStencilState* DepthStencilState = nullptr;
    ID3D11BlendState* BlendState = nullptr;
    FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    UINT sampleMask = 0xFFFFFFFF;

public:
    /**
     * Binds this effect's pipeline state to the provided device context
     * @param context The Direct3D device context to bind to
     */
    void bindPipeline(ID3D11DeviceContext* context);

    /**
     * Initializes pipeline states with default values
     * @param device The Direct3D device for creating state objects
     */
    void initDefaultStates(ID3D11Device* device);

    /**
     * Creates an effect with pre-loaded shaders
     * @param device The Direct3D device
     * @param vertexShader Pre-compiled vertex shader
     * @param pixelShader Pre-compiled pixel shader
     * @param inputLayout Pre-created input layout
     */
    Effect(ID3D11Device* device,
        ID3D11VertexShader* vertexShader,
        ID3D11PixelShader* pixelShader,
        ID3D11InputLayout* inputLayout);

    /**
     * Creates an effect by loading shaders from files
     * @param device The Direct3D device
     * @param vertexShaderPath Path to compiled vertex shader file
     * @param pixelShaderPath Path to compiled pixel shader file
     * @param vertexDesc Array of input element descriptions
     * @param numVertexElements Number of elements in vertexDesc array
     */
    Effect(ID3D11Device* device,
        const char* vertexShaderPath,
        const char* pixelShaderPath,
        const D3D11_INPUT_ELEMENT_DESC vertexDesc[],
        UINT numVertexElements);

    /**
     * Copy constructor - creates an effect by copying another effect's state
     * @param sourceEffect Source effect to copy from
     */
    Effect(std::shared_ptr<Effect> sourceEffect) {
        if (!sourceEffect) {
            throw std::invalid_argument("Source effect cannot be null");
        }

        // Copy pipeline input layout
        VSInputLayout = sourceEffect->VSInputLayout;
        if (VSInputLayout) {
            VSInputLayout->AddRef();
        }

        // Copy pipeline shaders
        VertexShader = sourceEffect->VertexShader;
        if (VertexShader) {
            VertexShader->AddRef();
        }

        PixelShader = sourceEffect->PixelShader;
        if (PixelShader) {
            PixelShader->AddRef();
        }

        GeometryShader = sourceEffect->GeometryShader;
        if (GeometryShader) {
            GeometryShader->AddRef();
        }

        HullShader = sourceEffect->HullShader;
        if (HullShader) {
            HullShader->AddRef();
        }

        DomainShader = sourceEffect->DomainShader;
        if (DomainShader) {
            DomainShader->AddRef();
        }

        // Copy pipeline states
        RasterizerState = sourceEffect->RasterizerState;
        if (RasterizerState) {
            RasterizerState->AddRef();
        }

        DepthStencilState = sourceEffect->DepthStencilState;
        if (DepthStencilState) {
            DepthStencilState->AddRef();
        }

        BlendState = sourceEffect->BlendState;
        if (BlendState) {
            BlendState->AddRef();
        }

        // Copy blend factors and sample mask
        for (int i = 0; i < 4; i++) {
            blendFactor[i] = sourceEffect->blendFactor[i];
        }
        sampleMask = sourceEffect->sampleMask;
    }

    /**
     * Destructor - releases all resources
     */
    ~Effect();

    // Getter methods
    ID3D11InputLayout* getVSInputLayout() const { return VSInputLayout; }
    ID3D11VertexShader* getVertexShader() const { return VertexShader; }
    ID3D11PixelShader* getPixelShader() const { return PixelShader; }
    ID3D11GeometryShader* getGeometryShader() const { return GeometryShader; }
    ID3D11RasterizerState* getRasterizerState() const { return RasterizerState; }
    ID3D11DepthStencilState* getDepthStencilState() const { return DepthStencilState; }
    ID3D11BlendState* getBlendState() const { return BlendState; }

    // Setter methods - Note: These don't AddRef, so caller must ensure objects stay valid
    void setPixelShader(ID3D11PixelShader* pixelShader) {
        if (PixelShader == pixelShader) return;
        SafeRelease(PixelShader);
        PixelShader = pixelShader;
        if (PixelShader) PixelShader->AddRef();
    }

    void setGeometryShader(ID3D11GeometryShader* geometryShader) {
        if (GeometryShader == geometryShader) return;
        SafeRelease(GeometryShader);
        GeometryShader = geometryShader;
        if (GeometryShader) GeometryShader->AddRef();
    }

    void setVertexShader(ID3D11VertexShader* vertexShader) {
        if (VertexShader == vertexShader) return;
        SafeRelease(VertexShader);
        VertexShader = vertexShader;
        if (VertexShader) VertexShader->AddRef();
    }

    void setVSInputLayout(ID3D11InputLayout* inputLayout) {
        if (VSInputLayout == inputLayout) return;
        SafeRelease(VSInputLayout);
        VSInputLayout = inputLayout;
        if (VSInputLayout) VSInputLayout->AddRef();
    }

    void setRasterizerState(ID3D11RasterizerState* rasterizerState) {
        if (RasterizerState == rasterizerState) return;
        SafeRelease(RasterizerState);
        RasterizerState = rasterizerState;
        if (RasterizerState) RasterizerState->AddRef();
    }

    void setDepthStencilState(ID3D11DepthStencilState* depthStencilState) {
        if (DepthStencilState == depthStencilState) return;
        SafeRelease(DepthStencilState);
        DepthStencilState = depthStencilState;
        if (DepthStencilState) DepthStencilState->AddRef();
    }

    void setBlendState(ID3D11BlendState* blendState) {
        if (BlendState == blendState) return;
        SafeRelease(BlendState);
        BlendState = blendState;
        if (BlendState) BlendState->AddRef();
    }

    /**
     * Sets the culling mode for the rasterizer state
     * @param device The Direct3D device
     * @param cullMode The culling mode to set
     */
    void setCullMode(ID3D11Device* device, D3D11_CULL_MODE cullMode) {
        if (!device || !RasterizerState) {
            return;
        }

        D3D11_RASTERIZER_DESC rsDesc;
        RasterizerState->GetDesc(&rsDesc);

        if (rsDesc.CullMode == cullMode) {
            return; // No change needed
        }

        rsDesc.CullMode = cullMode;

        ID3D11RasterizerState* newState = nullptr;
        HRESULT hr = device->CreateRasterizerState(&rsDesc, &newState);

        if (SUCCEEDED(hr)) {
            SafeRelease(RasterizerState);
            RasterizerState = newState;
        }
    }

    /**
     * Sets the depth write mask for depth-stencil state
     * @param device The Direct3D device
     * @param depthWriteMask The depth write mask to set
     */
    void setDepthWriteMask(ID3D11Device* device, D3D11_DEPTH_WRITE_MASK depthWriteMask) {
        if (!device || !DepthStencilState) {
            return;
        }

        D3D11_DEPTH_STENCIL_DESC dsDesc;
        DepthStencilState->GetDesc(&dsDesc);

        if (dsDesc.DepthWriteMask == depthWriteMask) {
            return; // No change needed
        }

        dsDesc.DepthWriteMask = depthWriteMask;

        ID3D11DepthStencilState* newState = nullptr;
        HRESULT hr = device->CreateDepthStencilState(&dsDesc, &newState);

        if (SUCCEEDED(hr)) {
            SafeRelease(DepthStencilState);
            DepthStencilState = newState;
        }
    }

    /**
     * Sets the depth comparison function for depth-stencil state
     * @param device The Direct3D device
     * @param depthFunc The depth comparison function to set
     */
    void setDepthFunction(ID3D11Device* device, D3D11_COMPARISON_FUNC depthFunc) {
        if (!device || !DepthStencilState) {
            return;
        }

        D3D11_DEPTH_STENCIL_DESC dsDesc;
        DepthStencilState->GetDesc(&dsDesc);

        if (dsDesc.DepthFunc == depthFunc) {
            return; // No change needed
        }

        dsDesc.DepthFunc = depthFunc;

        ID3D11DepthStencilState* newState = nullptr;
        HRESULT hr = device->CreateDepthStencilState(&dsDesc, &newState);

        if (SUCCEEDED(hr)) {
            SafeRelease(DepthStencilState);
            DepthStencilState = newState;
        }
    }

    /**
     * Enables or disables alpha-to-coverage
     * @param device The Direct3D device
     * @param enable Whether to enable alpha-to-coverage
     */
    void setAlphaToCoverage(ID3D11Device* device, bool enable) {
        if (!device || !BlendState) {
            return;
        }

        D3D11_BLEND_DESC bsDesc;
        BlendState->GetDesc(&bsDesc);

        // Check if already in desired state
        if (bsDesc.AlphaToCoverageEnable == (enable ? TRUE : FALSE)) {
            return;
        }

        bsDesc.AlphaToCoverageEnable = enable ? TRUE : FALSE;

        // Alpha-to-coverage and alpha blending are mutually exclusive
        if (enable) {
            bsDesc.RenderTarget[0].BlendEnable = FALSE;
        }

        ID3D11BlendState* newState = nullptr;
        HRESULT hr = device->CreateBlendState(&bsDesc, &newState);

        if (SUCCEEDED(hr)) {
            SafeRelease(BlendState);
            BlendState = newState;
        }
    }

    /**
     * Enables or disables alpha blending
     * @param device The Direct3D device
     * @param enable Whether to enable alpha blending
     */
    void setAlphaBlendEnable(ID3D11Device* device, bool enable) {
        if (!device || !BlendState) {
            return;
        }

        D3D11_BLEND_DESC bsDesc;
        BlendState->GetDesc(&bsDesc);

        // Check if already in desired state
        if (bsDesc.RenderTarget[0].BlendEnable == (enable ? TRUE : FALSE)) {
            return;
        }

        bsDesc.RenderTarget[0].BlendEnable = enable ? TRUE : FALSE;

        if (enable) {
            // Configure standard alpha blending
            bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            bsDesc.AlphaToCoverageEnable = FALSE; // Alpha blending and alpha-to-coverage are mutually exclusive
        }

        ID3D11BlendState* newState = nullptr;
        HRESULT hr = device->CreateBlendState(&bsDesc, &newState);

        if (SUCCEEDED(hr)) {
            SafeRelease(BlendState);
            BlendState = newState;
        }
    }

    // Shader creation methods
    /**
     * Creates a vertex shader from a file
     * @param device The Direct3D device
     * @param filename Path to the compiled shader file
     * @param vsBytecode Pointer to receive shader bytecode
     * @param vertexShader Pointer to receive created shader
     * @return Size in bytes of the shader bytecode
     */
    uint32_t CreateVertexShader(ID3D11Device* device,
        const char* filename,
        char** vsBytecode,
        ID3D11VertexShader** vertexShader);

    /**
     * Creates a pixel shader from a file
     * @param device The Direct3D device
     * @param filename Path to the compiled shader file
     * @param psBytecode Pointer to receive shader bytecode
     * @param pixelShader Pointer to receive created shader
     * @return HRESULT indicating success or failure
     */
    HRESULT CreatePixelShader(ID3D11Device* device,
        const char* filename,
        char** psBytecode,
        ID3D11PixelShader** pixelShader);

    /**
     * Creates a geometry shader from a file
     * @param device The Direct3D device
     * @param filename Path to the compiled shader file
     * @param gsBytecode Pointer to receive shader bytecode
     * @param geometryShader Pointer to receive created shader
     * @return HRESULT indicating success or failure
     */
    HRESULT CreateGeometryShader(ID3D11Device* device,
        const char* filename,
        char** gsBytecode,
        ID3D11GeometryShader** geometryShader);

    /**
     * Creates a hull shader from a file
     * @param device The Direct3D device
     * @param filename Path to the compiled shader file
     * @param hsBytecode Pointer to receive shader bytecode
     * @param hullShader Pointer to receive created shader
     * @return HRESULT indicating success or failure
     */
	HRESULT Effect::CreateHullShader(ID3D11Device *device, 
        const char *filename, 
        char **GSBytecode, 
        ID3D11HullShader **hullShader);
    /**
     * Creates a hull shader from a file
     * @param device The Direct3D device
     * @param filename Path to the compiled shader file
     * @param gsBytecode Pointer to receive shader bytecode
     * @param geometryShader Pointer to receive created shader
     * @return HRESULT indicating success or failure
     */
	HRESULT Effect::CreateDomainShader(ID3D11Device *device,
        const char *filename,
        char **GSBytecode,
        ID3D11DomainShader **domainShader);

};

