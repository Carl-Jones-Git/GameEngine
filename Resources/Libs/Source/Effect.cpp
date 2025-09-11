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
#include "Effect.h"

void Effect::bindPipeline(ID3D11DeviceContext* context) {
    if (!context) {
        return;
    }

    // Set pipeline states
    context->RSSetState(RasterizerState);
    context->OMSetDepthStencilState(DepthStencilState, 0);
    context->OMSetBlendState(BlendState, blendFactor, sampleMask);

    // Set shaders
    context->VSSetShader(VertexShader, nullptr, 0);
    context->PSSetShader(PixelShader, nullptr, 0);
    context->GSSetShader(GeometryShader, nullptr, 0);
    context->DSSetShader(DomainShader, nullptr, 0);
    context->HSSetShader(HullShader, nullptr, 0);

    // Set input layout
    context->IASetInputLayout(VSInputLayout);
}

void Effect::initDefaultStates(ID3D11Device* device) {
    if (!device) {
        throw std::invalid_argument("Device cannot be null");
    }

    HRESULT hr;

    // Initialize default rasterizer state
    D3D11_RASTERIZER_DESC rsDesc = {};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthBias = 0;
    rsDesc.DepthBiasClamp = 0.0f;
    rsDesc.SlopeScaledDepthBias = 1.0f;
    rsDesc.DepthClipEnable = TRUE;
    rsDesc.ScissorEnable = FALSE;
    rsDesc.MultisampleEnable = TRUE;
    rsDesc.AntialiasedLineEnable = TRUE;

    hr = device->CreateRasterizerState(&rsDesc, &RasterizerState);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create rasterizer state");
    }

    // Initialize default depth-stencil state
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.StencilEnable = FALSE;
    dsDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    dsDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

    // Front face stencil operations
    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

    // Back face stencil operations (same as front face in this default setup)
    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

    hr = device->CreateDepthStencilState(&dsDesc, &DepthStencilState);
    if (FAILED(hr)) {
        SafeRelease(RasterizerState);
        throw std::runtime_error("Failed to create depth-stencil state");
    }

    // Initialize default blend state (Alpha blending enabled by default)
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = TRUE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = device->CreateBlendState(&blendDesc, &BlendState);
    if (FAILED(hr)) {
        SafeRelease(RasterizerState);
        SafeRelease(DepthStencilState);
        throw std::runtime_error("Failed to create blend state");
    }

    // Initialize blend factors and sample mask
    blendFactor[0] = blendFactor[1] = blendFactor[2] = blendFactor[3] = 1.0f;
    sampleMask = 0xFFFFFFFF; // Process all samples in MSAA
}

Effect::Effect(ID3D11Device* device, ID3D11VertexShader* vertexShader, ID3D11PixelShader* pixelShader, ID3D11InputLayout* inputLayout) {
    if (!device || !vertexShader || !pixelShader || !inputLayout) {
        throw std::invalid_argument("Invalid shader or input layout pointers");
    }

    // Store and add references to provided shader objects
    VertexShader = vertexShader;
    PixelShader = pixelShader;
    VSInputLayout = inputLayout;

    VertexShader->AddRef();
    PixelShader->AddRef();
    VSInputLayout->AddRef();

    // Initialize pipeline states with defaults
    initDefaultStates(device);
}

Effect::Effect(ID3D11Device* device, const char* vertexShaderPath, const char* pixelShaderPath,
    const D3D11_INPUT_ELEMENT_DESC vertexDesc[], UINT numVertexElements) {
    if (!device || !vertexShaderPath || !pixelShaderPath || !vertexDesc) {
        throw std::invalid_argument("Invalid shader path or vertex description");
    }

    char* shaderBytecode = nullptr;
    HRESULT hr;

    try {
        // Load and create vertex shader
        uint32_t vsSizeBytes = CreateVertexShader(device, vertexShaderPath, &shaderBytecode, &VertexShader);
        if (!shaderBytecode || !VertexShader) {
            throw std::runtime_error("Failed to create vertex shader");
        }

        // Create input layout from vertex shader bytecode
        hr = device->CreateInputLayout(vertexDesc, numVertexElements, shaderBytecode, vsSizeBytes, &VSInputLayout);
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create input layout");
        }

        // Free vertex shader bytecode
        free(shaderBytecode);
        shaderBytecode = nullptr;

        // Load and create pixel shader
        hr = CreatePixelShader(device, pixelShaderPath, &shaderBytecode, &PixelShader);
        if (FAILED(hr) || !PixelShader) {
            throw std::runtime_error("Failed to create pixel shader");
        }

        // Free pixel shader bytecode
        free(shaderBytecode);
        shaderBytecode = nullptr;

        // Initialize pipeline states with defaults
        initDefaultStates(device);

    }
    catch (const std::exception& e) {
        // Clean up resources in case of failure
        if (shaderBytecode) {
            free(shaderBytecode);
        }
        SafeRelease(VertexShader);
        SafeRelease(PixelShader);
        SafeRelease(VSInputLayout);
        throw; // Re-throw the caught exception
    }
}

Effect::~Effect() {
    // Release all DirectX resources
    SafeRelease(RasterizerState);
    SafeRelease(DepthStencilState);
    SafeRelease(BlendState);
    SafeRelease(VertexShader);
    SafeRelease(PixelShader);
    SafeRelease(GeometryShader);
    SafeRelease(HullShader);
    SafeRelease(DomainShader);
    SafeRelease(VSInputLayout);
}

uint32_t Effect::CreateVertexShader(ID3D11Device* device, const char* filename,
    char** vsBytecode, ID3D11VertexShader** vertexShader) {
    if (!device || !filename || !vsBytecode || !vertexShader) {
        throw std::invalid_argument("Invalid parameters for vertex shader creation");
    }

    std::cout << "Loading Vertex Shader: " << filename << std::endl;

    // Load compiled shader bytecode
    uint32_t shaderBytes = LoadShader(filename, vsBytecode);
    if (shaderBytes == 0 || !*vsBytecode) {
        throw std::runtime_error("Failed to load vertex shader bytecode");
    }

    // Create the vertex shader
    HRESULT hr = device->CreateVertexShader(*vsBytecode, shaderBytes, nullptr, vertexShader);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertex shader interface");
    }

    return shaderBytes;
}

HRESULT Effect::CreatePixelShader(ID3D11Device* device, const char* filename,
    char** psBytecode, ID3D11PixelShader** pixelShader) {
    if (!device || !filename || !psBytecode || !pixelShader) {
        return E_INVALIDARG;
    }

    std::cout << "Loading Pixel Shader: " << filename << std::endl;

    // Load compiled shader bytecode
    uint32_t shaderBytes = LoadShader(filename, psBytecode);
    if (shaderBytes == 0 || !*psBytecode) {
        return E_FAIL;
    }

    // Create the pixel shader
    HRESULT hr = device->CreatePixelShader(*psBytecode, shaderBytes, nullptr, pixelShader);
    if (FAILED(hr)) {
        std::cerr << "Failed to create pixel shader. HRESULT: 0x" << std::hex << hr << std::endl;
    }

    return hr;
}

HRESULT Effect::CreateGeometryShader(ID3D11Device* device, const char* filename,
    char** gsBytecode, ID3D11GeometryShader** geometryShader) {
    if (!device || !filename || !gsBytecode || !geometryShader) {
        return E_INVALIDARG;
    }

    std::cout << "Loading Geometry Shader: " << filename << std::endl;

    // Load compiled shader bytecode
    uint32_t shaderBytes = LoadShader(filename, gsBytecode);
    if (shaderBytes == 0 || !*gsBytecode) {
        return E_FAIL;
    }

    std::cout << "Loaded GShader bytecode: " << shaderBytes << " bytes" << std::endl;

    // Create the geometry shader
    HRESULT hr = device->CreateGeometryShader(*gsBytecode, shaderBytes, nullptr, geometryShader);
    if (FAILED(hr)) {
        std::cerr << "Failed to create geometry shader. HRESULT: 0x" << std::hex << hr << std::endl;
    }

    return hr;
}

HRESULT Effect::CreateHullShader(ID3D11Device* device, const char* filename,
    char** hsBytecode, ID3D11HullShader** hullShader) {
    if (!device || !filename || !hsBytecode || !hullShader) {
        return E_INVALIDARG;
    }

    std::cout << "Loading Hull Shader: " << filename << std::endl;

    // Load compiled shader bytecode
    uint32_t shaderBytes = LoadShader(filename, hsBytecode);
    if (shaderBytes == 0 || !*hsBytecode) {
        return E_FAIL;
    }

    // Create the hull shader
    HRESULT hr = device->CreateHullShader(*hsBytecode, shaderBytes, nullptr, hullShader);
    if (FAILED(hr)) {
        std::cerr << "Failed to create hull shader. HRESULT: 0x" << std::hex << hr << std::endl;
    }

    return hr;
}

HRESULT Effect::CreateDomainShader(ID3D11Device* device, const char* filename,
    char** dsBytecode, ID3D11DomainShader** domainShader) {
    if (!device || !filename || !dsBytecode || !domainShader) {
        return E_INVALIDARG;
    }

    std::cout << "Loading Domain Shader: " << filename << std::endl;

    // Load compiled shader bytecode
    uint32_t shaderBytes = LoadShader(filename, dsBytecode);
    if (shaderBytes == 0 || !*dsBytecode) {
        return E_FAIL;
    }

    // Create the domain shader
    HRESULT hr = device->CreateDomainShader(*dsBytecode, shaderBytes, nullptr, domainShader);
    if (FAILED(hr)) {
        std::cerr << "Failed to create domain shader. HRESULT: 0x" << std::hex << hr << std::endl;
    }

    return hr;
}