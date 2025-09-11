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
#include <Grid.h>
#include <Material.h>

HRESULT  Grid::init(ID3D11Device *device, UINT widthl, UINT heightl)
{

	Material material;
	XMCOLOR specular = XMCOLOR(1.0f, 1.0f, 1.0, 1.0);
	XMCOLOR diffuse = XMCOLOR(1.0f, 1.0f, 1.0, 1.0);

	if (instances[0].materials.size()>0)
	{
		shared_ptr<Material>material = instances[0].materials[0];
		diffuse = XMCOLOR(material->getMaterialProperties()->diffuse.x, material->getMaterialProperties()->diffuse.y, material->getMaterialProperties()->diffuse.z, material->getMaterialProperties()->diffuse.w);
		specular = XMCOLOR(material->getMaterialProperties()->specular.x, material->getMaterialProperties()->specular.y, material->getMaterialProperties()->specular.z, material->getMaterialProperties()->specular.w);
	}

	width = widthl;
	height = heightl;
	numInd = ((width - 1) * 2 * 3)*(height - 1);
	ExtendedVertexStruct*vertices = (ExtendedVertexStruct*)malloc(sizeof(ExtendedVertexStruct)*width*height);
	UINT*indices = (UINT*)malloc(sizeof(UINT)*numInd);
	
	try
	{

		// Setup grid vertex buffer

		//INITIALISE Verticies
		for (int i = 0; i<height; i++)
		{
			for (int j = 0; j<width; j++)
			{

				vertices[(i*width) + j].pos.x = j;
				vertices[(i*width) + j].pos.z = i ;
				vertices[(i*width) + j].pos.y = 0;
				vertices[(i*width) + j].normal=XMFLOAT3(0, 1, 0);
				vertices[(i*width) + j].matDiffuse = diffuse;
				vertices[(i*width) + j].matSpecular = specular;
				vertices[(i*width) + j].texCoord.x = (float)j / width;
				vertices[(i*width) + j].texCoord.y = (float)i / height;
			}
		}



		// Setup vertex buffer
		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexDesc.ByteWidth = sizeof(ExtendedVertexStruct) * width*height;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexData.pSysMem = vertices;

		HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer cannot be created");


		for (int i = 0; i<height - 1; i++)
		{
			for (int j = 0; j<width - 1; j++)
			{
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 0] = (i*width) + j;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 2] = (i*width) + j + 1;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 1] = ((i + 1)*width) + j;

				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 3] = (i*width) + j + 1;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 5] = ((i + 1)*width) + j + 1;;
				indices[(i*(width - 1) * 2 * 3) + (j * 2 * 3) + 4] = ((i + 1)*width) + j;
			}
		}


		D3D11_BUFFER_DESC indexDesc;
		indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexDesc.ByteWidth = sizeof(UINT)* numInd;
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexDesc.CPUAccessFlags = 0;
		indexDesc.MiscFlags = 0;
		indexDesc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA indexData;
		indexData.pSysMem = indices;
		
		hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);
		
		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer cannot be created");

		// Dispose of local resources
		if (vertices)
			free(vertices);
		if (indices)
			free(indices);

	}
	catch (exception& e)
	{
		cout << "Grid object could not be instantiated due to:\n";
		cout << e.what() << endl;

		if (vertexBuffer)
			vertexBuffer->Release();
		if (indexBuffer)
			indexBuffer->Release();
		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		return E_FAIL;
	}
	return S_OK;
}



Grid::~Grid() {
//Buffers cleaned up by BaseModel
}


void Grid::render(ID3D11DeviceContext *context, int instanceIndex) {

	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);

	// Validate object before rendering 
	if (!context || !vertexBuffer )
		return;

	if (effect)
		// Sets shaders, states
		effect->bindPipeline(context);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (instances[instanceIndex].materials.size() > 0)
		instances[instanceIndex].materials[0]->update(context);
	if (sampler) {

		//context->PSSetShaderResources(0, numTextures, textures);
		context->PSSetSamplers(0, 1, &sampler);
	}

	// Set vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ExtendedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);

	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw box object using index buffer
	// 36 indices for the box.
	context->DrawIndexed(numInd, 0, 0);

}