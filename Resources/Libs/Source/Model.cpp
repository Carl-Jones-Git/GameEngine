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
#include "Model.h"
#include <Utils.h>
#include <Material.h>
#include <Effect.h>
#include <iostream>
#include <exception>
#include <locale> 
#include <codecvt>

using namespace std;
using  DirectX::XMFLOAT2;
using  DirectX::XMFLOAT3;
using namespace DirectX::PackedVector;

void Model::load(ID3D11Device *device,  const std::wstring& filename) {

	try
	{
		if (!device )
			throw exception("Invalid parameters for Model instantiation");

		HRESULT hr = 0;

		if (!SUCCEEDED(hr))
			throw exception("Cannot create input layout interface");
	
	}
	catch (exception& e)
	{
		cout << "Model could not be instantiated due to:\n";
		cout << e.what() << endl;

		if (vertexBuffer)
			vertexBuffer->Release();

		if (indexBuffer)
			indexBuffer->Release();

		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		numMeshes = 0;
	}
}

Model::~Model() {
	if (indexBufferCPU)
		free(indexBufferCPU);
	if (vertexBufferCPU)
		free(vertexBufferCPU);
}


void Model::render(ID3D11DeviceContext *context,int instanceIndex) {//, int mode

	// Validate Model before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !indexBuffer || !effect)
		return;

	effect->bindPipeline(context);

	// Set Model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ExtendedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (sampler)
		context->PSSetSamplers(0, 1, &sampler);

	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);

	// Draw Model
	for (uint32_t indexOffset = 0, i = 0; i < numMeshes; indexOffset += indexCount[i], ++i)
	{
		if (instances[instanceIndex].materials.size() > i)
			instances[instanceIndex].materials[i]->update(context);
		context->DrawIndexed(indexCount[i], indexOffset, baseVertexOffset[i]);
	}	
}


HRESULT Model::loadModelAssimp(ID3D11DeviceContext* context, ID3D11Device *device, const std::wstring& filename, XMMATRIX* preTrans)
{
	//Assimp::Importer importer;
	std::string filename_string = WStringToString(filename);
		
	// Get filename extension
	wstring ext = filename.substr(filename.length() - 4);

	try
	{
		//as well as specifying the mesh to load ,meshNumber is used control loading flags this is a bodge that needs fixing
		if (meshNumber == NO_JOIN)
			scene = importer.ReadFile(filename_string, aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
				aiProcess_Triangulate);
		else if (meshNumber == NO_PRETRANS)
			scene = importer.ReadFile(filename_string,aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			aiProcess_SortByPType);
		else if (meshNumber == DEFAULT)
			scene = importer.ReadFile(filename_string, aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
				aiProcess_Triangulate |
				aiProcess_JoinIdenticalVertices |
				aiProcess_SortByPType);

		else //we are loading a specific mesh
			scene = importer.ReadFile(filename_string, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
				aiProcess_JoinIdenticalVertices |
				aiProcess_SortByPType);
		

		if (!scene)
		{
			printf("Couldn't load model - Error Importing Asset");
			return  E_FAIL;
		}

		if (instances[0].materials.size()<1)//Else materials have already been defined and overide the materials from the file
		{
			//Load the diffuse texture for each mesh in the file
			for (int i = 0; i < min(scene->mNumMeshes, MAX_TEXTURES); i++)
			{
				shared_ptr< Material> material(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
				material->setSpecular(XMFLOAT4(0.1, 0.1, 0.1, 0.1));

				aiMaterial* ASSIMPmaterial = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
				if (ASSIMPmaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
				{
					aiString diffuseFileName;

					ASSIMPmaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseFileName);
					// Copy string to wstring.
					wstring diffuseFileNameW = StringToWString(diffuseFileName.C_Str());
					wstring directoryW = filename.substr(0, filename.find_last_of(L"/\\"));
					Texture texture = Texture(context, device, directoryW + L"\\" + diffuseFileNameW);
					cout << "loading texture: " << WStringToString(directoryW + L"\\" + diffuseFileNameW) << endl;

					if (texture.getShaderResourceView())
					{
						material->setUsage(DIFFUSE_MAP);
						material->setTexture(texture.getShaderResourceView());
					}
				}

				if (instances[0].materials.size() <= i)
					instances[0].materials.push_back(material);		
				else
					instances[0].materials[i] = material;			
			}	
		}

		XMCOLOR diffuse = XMCOLOR(1, 1, 1, 1);
		XMCOLOR specular = XMCOLOR(1, 1, 1, 1);

		numMeshes = scene->mNumMeshes;

		if (numMeshes == 0)
			throw exception("Empty model loaded");

		uint32_t numVertices = 0;
		uint32_t numIndices = 0;
		for (uint32_t k = 0; k < numMeshes; ++k)
		{
			int numIndicesL = 0;
			if (meshNumber == k || meshNumber<=-1)
			{
				aiMesh* mesh = scene->mMeshes[k];
				//Store base vertex index;
				baseVertexOffset.push_back(numVertices);
				// Increment vertex count
				numVertices += mesh->mNumVertices;
				// Store num indices for current mesh
				for (int i = 0; i < mesh->mNumFaces; i++)
				{
					numIndicesL += mesh->mFaces[i].mNumIndices;
					numIndices += mesh->mFaces[i].mNumIndices;
				}
				indexCount.push_back(numIndicesL);
			}
		}



		// Create vertex buffer
		vertexBufferCPU = (ExtendedVertexStruct*)malloc(numVertices * sizeof(ExtendedVertexStruct));

			if (!vertexBufferCPU)
				throw exception("Cannot create vertex buffer");

			// Create index buffer
			indexBufferCPU = (uint32_t*)malloc(numIndices * sizeof(uint32_t));

			if (!indexBufferCPU)
				throw exception("Cannot create index buffer");


			// Copy vertex data into single buffer
			ExtendedVertexStruct *vptr = vertexBufferCPU;
			uint32_t *indexPtr = indexBufferCPU;
			cout << filename_string.substr(filename_string.find_last_of("/\\")+1, 12).c_str() << endl;

			if (meshNumber > -1)//we are loading a single mesh
				numMeshes = 1;
			for (uint32_t i = 0; i < numMeshes; ++i) 
			{
				int currentMesh = i;
				if (meshNumber > -1)
					currentMesh = meshNumber;

				if (instances[0].materials.size() > i)
				{
					diffuse = XMCOLOR(instances[0].materials[i]->getMaterialProperties()->diffuse.x, instances[0].materials[i]->getMaterialProperties()->diffuse.y, instances[0].materials[i]->getMaterialProperties()->diffuse.z, instances[0].materials[i]->getMaterialProperties()->diffuse.w);
					specular = XMCOLOR(instances[0].materials[i]->getMaterialProperties()->specular.x, instances[0].materials[i]->getMaterialProperties()->specular.y, instances[0].materials[i]->getMaterialProperties()->specular.z, instances[0].materials[i]->getMaterialProperties()->specular.w);
				}


				aiMesh* mesh = scene->mMeshes[currentMesh];
				uint32_t j = 0;
				for ( j = 0; j < mesh->mNumFaces; ++j)		
				{
					const aiFace& face = mesh->mFaces[j];
					for (int k = 0; k < face.mNumIndices; ++k)
					{
						int VIndex = baseVertexOffset[i] + face.mIndices[k];
						aiVector3D pos = mesh->mVertices[face.mIndices[k]];
						aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][face.mIndices[k]] : aiVector3D(1.0f, 1.0f, 1.0f);
						//A hack to select the second set of uv coords only if we are loading the dragon
						if (0 == ext.compare(L".fbx") && 0==filename.substr(filename.find_last_of(L"/\\")+1, 12).compare(L"Dragon_Baked"))
							uv = mesh->mTextureCoords[1][face.mIndices[k]];
						aiVector3D normal = mesh->HasNormals() ? mesh->mNormals[face.mIndices[k]] : aiVector3D(1.0f, 1.0f, 1.0f);
						aiVector3D tangent = mesh->HasTangentsAndBitangents() ? mesh->mTangents[face.mIndices[k]] : aiVector3D(1.0f, 1.0f, 1.0f);
						if (preTrans)//transform all vertices by the preTrans matrix provided
						{
							XMVECTOR posTrans = DirectX::XMVector3TransformCoord(XMVectorSet(pos.x, pos.y, pos.z, 1.0), *preTrans);
							vptr[VIndex].pos = XMFLOAT3(XMVectorGetX(posTrans), XMVectorGetY(posTrans), XMVectorGetZ(posTrans));
							XMVECTOR normTrans = DirectX::XMVector3TransformNormal(XMVectorSet(normal.x, normal.y, normal.z, 1.0), *preTrans);
							vptr[VIndex].normal = XMFLOAT3(XMVectorGetX(normTrans), XMVectorGetY(normTrans), XMVectorGetZ(normTrans));
							XMVECTOR tangTrans = DirectX::XMVector3TransformCoord(XMVectorSet(tangent.x, tangent.y, tangent.z, 1.0), *preTrans);
							vptr[VIndex].tangent = XMFLOAT3(XMVectorGetX(tangTrans), XMVectorGetY(tangTrans), XMVectorGetZ(tangTrans));
						}
						else
						{
							vptr[VIndex].pos = XMFLOAT3(pos.x, pos.y, pos.z);
							vptr[VIndex].normal = XMFLOAT3(normal.x, normal.y, normal.z);
							vptr[VIndex].tangent = XMFLOAT3(tangent.x, tangent.y, tangent.z);
						}
						if (0 == ext.compare(L".3ds"))
							vptr[VIndex].texCoord = XMFLOAT2(uv.x, uv.y);
						else
						vptr[VIndex].texCoord = XMFLOAT2(uv.x, 1.0-uv.y);
						vptr[VIndex].matDiffuse = diffuse;		
						//A hack for loading the racing line. The colour of the vertices is stored in the normals
						wstring tmp = filename.substr(filename.find_last_of(L"/\\") + 1, 6);
						if (tmp.compare(L"Racing") == 0)
							vptr[VIndex].matDiffuse = XMCOLOR(normal.x, normal.y, normal.z, 1.0f);
						vptr[VIndex].matSpecular = specular;
						indexPtr[0] = face.mIndices[k];
						indexPtr++;

					}
				}//for each face	
			}//for each mesh

			num_vert = numVertices;
			// Setup DX vertex buffer interfaces
			D3D11_BUFFER_DESC vertexDesc;
			D3D11_SUBRESOURCE_DATA vertexData;

			ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
			ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

			vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
			vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexDesc.ByteWidth = numVertices * sizeof(ExtendedVertexStruct);
			vertexData.pSysMem = vertexBufferCPU;

			HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

			if (!SUCCEEDED(hr))
				throw exception("Vertex buffer cannot be created");

			num_ind = numIndices;

			// Setup index buffer
			D3D11_BUFFER_DESC indexDesc;
			D3D11_SUBRESOURCE_DATA indexData;

			ZeroMemory(&indexDesc, sizeof(D3D11_BUFFER_DESC));
			ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));

			indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
			indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
			indexDesc.ByteWidth = numIndices * sizeof(uint32_t);
			indexData.pSysMem = indexBufferCPU;

			hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

			if (!SUCCEEDED(hr))
				throw exception("Index buffer cannot be created");
		}
		catch (exception& e)
		{
			cout << "Model could not be instantiated due to:\n";
			cout << e.what() << endl;
			if (indexBufferCPU)
				free(indexBufferCPU);
			if (vertexBufferCPU)
				free(vertexBufferCPU);
			return-1;
		}
	return 0;
}

