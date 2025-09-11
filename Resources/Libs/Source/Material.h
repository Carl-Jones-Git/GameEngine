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
#include "Utils.h"
#include <memory>
#include <stdexcept>
#include <iostream>

/**
 * Maximum number of textures supported per material
 */
#define MAX_TEXTURES 8

 /**
  * Material usage flags - can be combined with bitwise OR
  */
enum MaterialUsage {
    USE_MATERIAL_ALL = 0,      // Use all material properties
    DIFFUSE_MAP = 1 << 0, // Use diffuse texture map
    NORMAL_MAP = 1 << 1, // Use normal texture map
    SPECULAR_MAP = 1 << 2, // Use specular texture map
    OPACITY_MAP = 1 << 3, // Use opacity/alpha texture map
    REFLECTION_MAP = 1 << 4, // Use reflection texture map
    EMISSIVE_MAP = 1 << 5, // Use emissive texture map
    CHROME = 1 << 8  // Chrome material preset
};

/**
 * Material class - handles material properties and textures for rendering
 */
class Material {
private:
    // CPU and GPU buffers for material properties
    CBufferMaterial* cBufferMaterialCPU = nullptr;
    ID3D11Buffer* cBufferMaterialGPU = nullptr;

    // Array of texture shader resource views
    ID3D11ShaderResourceView* textureSRVArray[MAX_TEXTURES];
    int numTextures = 0;

    /**
     * Initialize default material values
     */
    void initDefaultMaterialValues();

public:
    /**
     * Default constructor
     */
    Material();

    /**
     * Create material with default properties
     * @param device DirectX device for resource creation
     */
    Material(ID3D11Device* device);

    /**
     * Create material with custom color and usage flags
     * @param device DirectX device for resource creation
     * @param color RGBA color for ambient and diffuse components
     * @param use Material usage flags (default: USE_MATERIAL_ALL)
     */
    Material(ID3D11Device* device, XMFLOAT4 color, INT use = USE_MATERIAL_ALL);

    /**
     * Create material by copying properties from another material
     * @param device DirectX device for resource creation
     * @param sourceMaterial Source material to copy from
     */
    Material(ID3D11Device* device, std::shared_ptr<Material> sourceMaterial);

    /**
     * Destructor - releases all resources
     */
    ~Material();

    /**
     * Get the material properties
     * @return Pointer to material constant buffer structure
     */
    CBufferMaterial* getMaterialProperties() { return cBufferMaterialCPU; }

    /**
     * Set a texture at the specified index
     * @param textureSRV Shader resource view for the texture
     * @param index Texture array index (default: 0)
     */
    void setTexture(ID3D11ShaderResourceView* textureSRV, int index = 0) {
        if (index < 0 || index >= MAX_TEXTURES) {
            return;
        }

        // Release previous texture if it exists
        if (textureSRVArray[index]) {
            textureSRVArray[index]->Release();
        }

        textureSRVArray[index] = textureSRV;

        // AddRef to maintain proper reference count
        if (textureSRV) {
            textureSRV->AddRef();
        }

        // Update texture count
        numTextures = max(numTextures, index + 1);
    }

    /**
     * Set multiple textures at once
     * @param textures Array of shader resource views
     * @param count Number of textures to set
     */
    void setTextures(ID3D11ShaderResourceView* textures[], int count = 1) {
        if (!textures || count <= 0 || count > MAX_TEXTURES) {
            return;
        }

        // Release existing textures
        for (int i = 0; i < numTextures; i++) {
            if (textureSRVArray[i]) {
                textureSRVArray[i]->Release();
                textureSRVArray[i] = nullptr;
            }
        }

        // Set new textures
        numTextures = count;
        for (int i = 0; i < count; i++) {
            textureSRVArray[i] = textures[i];
            if (textureSRVArray[i]) {
                textureSRVArray[i]->AddRef();
            }
        }
    }

    /**
     * Get a texture at the specified index
     * @param index Texture array index
     * @return Shader resource view for the texture
     */
    ID3D11ShaderResourceView* getTexture(int index) {
        if (index < 0 || index >= MAX_TEXTURES) {
            return nullptr;
        }
        return textureSRVArray[index];
    }

    // Material property setters and getters
    void setEmissive(XMFLOAT4 emissive) { cBufferMaterialCPU->emissive = emissive; }
    void setAmbient(XMFLOAT4 ambient) { cBufferMaterialCPU->ambient = ambient; }
    void setDiffuse(XMFLOAT4 diffuse) { cBufferMaterialCPU->diffuse = diffuse; }
    XMFLOAT4 getDiffuse() { return cBufferMaterialCPU->diffuse; }
    void setSpecular(XMFLOAT4 specular) { cBufferMaterialCPU->specular = specular; }
    void setUsage(INT usage) { cBufferMaterialCPU->useage = usage; }

    /**
     * Update material constant buffer and bind resources
     * @param context DirectX device context
     */
    void update(ID3D11DeviceContext* context);

    /**
     * Initialize DirectX resources
     * @param device DirectX device
     */
    void init(ID3D11Device* device);
};
