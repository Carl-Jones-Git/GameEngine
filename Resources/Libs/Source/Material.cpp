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
#include "Material.h"

Material::Material() {
    // Default constructor intentionally left empty
}

Material::Material(ID3D11Device* device) {
    // Initialize with default material properties
    cBufferMaterialCPU = (CBufferMaterial*)_aligned_malloc(sizeof(CBufferMaterial), 16);
    if (!cBufferMaterialCPU) {
        throw std::bad_alloc();
    }

    // Set default material values
    initDefaultMaterialValues();
    cBufferMaterialCPU->useage = USE_MATERIAL_ALL;

    // Initialize DirectX resources
    init(device);
}

Material::Material(ID3D11Device* device, XMFLOAT4 color, INT use) {
    // Initialize with custom color and usage flags
    cBufferMaterialCPU = (CBufferMaterial*)_aligned_malloc(sizeof(CBufferMaterial), 16);
    if (!cBufferMaterialCPU) {
        throw std::bad_alloc();
    }

    // Set default values first
    initDefaultMaterialValues();

    // Override with custom color
    cBufferMaterialCPU->ambient = color;
    cBufferMaterialCPU->diffuse = color;
    cBufferMaterialCPU->useage = use;

    // Initialize DirectX resources
    init(device);
}

Material::Material(ID3D11Device* device, std::shared_ptr<Material> sourceMaterial) {
    // Initialize by copying from another material
    if (!sourceMaterial || !sourceMaterial->cBufferMaterialCPU) {
        throw std::invalid_argument("Source material is invalid");
    }

    cBufferMaterialCPU = (CBufferMaterial*)_aligned_malloc(sizeof(CBufferMaterial), 16);
    if (!cBufferMaterialCPU) {
        throw std::bad_alloc();
    }

    // Copy material properties
    memcpy(cBufferMaterialCPU, sourceMaterial->cBufferMaterialCPU, sizeof(CBufferMaterial));

    // Initialize DirectX resources
    init(device);
}

void Material::initDefaultMaterialValues() {
    cBufferMaterialCPU->emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    cBufferMaterialCPU->ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cBufferMaterialCPU->diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cBufferMaterialCPU->specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
}

void Material::update(ID3D11DeviceContext* context) {
    if (!context || !cBufferMaterialCPU || !cBufferMaterialGPU) {
        return;
    }

    // Update GPU buffer with current CPU buffer data
    mapCbuffer(context, cBufferMaterialCPU, cBufferMaterialGPU, sizeof(CBufferMaterial));

    // Bind material constant buffer to both vertex and pixel shaders
    context->VSSetConstantBuffers(4, 1, &cBufferMaterialGPU);
    context->PSSetConstantBuffers(4, 1, &cBufferMaterialGPU);

    // Set textures if available
    if (numTextures > 0) {
        for (int i = 0; i < numTextures; i++) {
            if (textureSRVArray[i] != nullptr) {
                context->PSSetShaderResources(0, numTextures, textureSRVArray);
                break; // We only need to set these once
            }
        }
    }
}

void Material::init(ID3D11Device* device) {
    if (!device) {
        return;
    }

    // Initialize texture array
    for (int i = 0; i < MAX_TEXTURES; i++) {
        textureSRVArray[i] = nullptr;
    }

    // Configure and create constant buffer
    D3D11_BUFFER_DESC cbufferDesc = {};
    cbufferDesc.ByteWidth = sizeof(CBufferMaterial);
    cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA cbufferInitData = {};
    cbufferInitData.pSysMem = cBufferMaterialCPU;

    HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferMaterialGPU);
    if (FAILED(hr)) {
        // Handle error - at minimum log it
        std::cerr << "Failed to create material constant buffer. HRESULT: " << hr << std::endl;
    }
}

Material::~Material() {
    // Release all textures
    for (int i = 0; i < MAX_TEXTURES; i++) {
        if (textureSRVArray[i]) {
            textureSRVArray[i]->Release();
            textureSRVArray[i] = nullptr;
        }
    }

    // Free aligned memory
    if (cBufferMaterialCPU) {
        _aligned_free(cBufferMaterialCPU);
        cBufferMaterialCPU = nullptr;
    }

    // Release DirectX resources
    if (cBufferMaterialGPU) {
        cBufferMaterialGPU->Release();
        cBufferMaterialGPU = nullptr;
    }
}