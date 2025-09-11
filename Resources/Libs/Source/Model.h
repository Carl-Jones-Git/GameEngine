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
#include <CBufferStructures.h>
#include <VertexStructures.h>
#include <BaseModel.h>
#include <Utils.h>
#include <Assimp\include\assimp\Importer.hpp>      // C++ importer interface
#include <Assimp\include\assimp\scene.h>           // Output data structure
#include <Assimp\include\assimp\postprocess.h>     // Post processing flags
#include <string>
#include <vector>
#include <cstdint>

class Texture;
class Material;
class Effect;
#define MAX_TEXTURES 8
using DirectX::XMMATRIX;
enum loadParams{ NO_JOIN=-3,NO_PRETRANS,DEFAULT};//0 and above we load a specific mesh

class Model : public BaseModel {
protected:
	//Protectd Attributes 
	uint32_t							numMeshes = 0;
	std::vector<uint32_t>				indexCount;
	std::vector<uint32_t>				baseVertexOffset;
	int	num_vert = 0;
	int	num_ind = 0;
	uint32_t* indexBufferCPU = nullptr;
	ExtendedVertexStruct* vertexBufferCPU = nullptr;

	//Assimp Attributes
	Assimp::Importer importer;
	const aiScene* scene = nullptr;

public:
	//Public Methods
	Model(ID3D11DeviceContext* context,ID3D11Device* device, const std::wstring& filename, shared_ptr<Effect>  _effect, std::shared_ptr<Material>_material = nullptr, int _meshNumber = DEFAULT,XMMATRIX *preTrans=nullptr) : BaseModel(device, _effect, _material, _meshNumber) { loadModelAssimp(context,device, filename, preTrans); }
	~Model();
	HRESULT init(ID3D11Device* device) { return S_OK; };
	HRESULT loadModelAssimp(ID3D11DeviceContext* context, ID3D11Device* device, const std::wstring& filename, XMMATRIX* preTrans = nullptr);
	void load(ID3D11Device* device, const std::wstring& filename);
	void render(ID3D11DeviceContext *context,int instanceIndex = 0);
	int getVBSizeBytes() { return num_vert * sizeof(ExtendedVertexStruct); }
	ID3D11Buffer* getIndices() { return  indexBuffer; }
	ID3D11Buffer* getVB() { return  vertexBuffer; }
	int getNumInd() { return 	num_ind; }
	int getNumFaces() { return 	num_ind / 3; }
	int getNumVert() { return 	num_vert; }
	int getNumMeshes() { return 	numMeshes; }
	ExtendedVertexStruct* getVertexBufferCPU() { return vertexBufferCPU; }
	uint32_t* getIndexBufferCPU() { return indexBufferCPU; }
	std::vector<uint32_t>	getIndexCount() {return indexCount;}
	std::vector<uint32_t>	getBaseVertexOffset() { return baseVertexOffset; }
};
