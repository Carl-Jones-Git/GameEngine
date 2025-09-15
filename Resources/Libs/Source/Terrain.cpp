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
#include "Terrain.h"
#include "Effect.h"
using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

DirectX::XMUINT4 Terrain::getMapColour(float x, float z)
{

	//z =-z;
	float p = x;
	x = z;
	z = p;
	// transform input from world coordinates to terrain model coordinates
	DirectX::XMVECTOR t = XMLoadFloat3(&XMFLOAT3(x, 0, z));
	XMMATRIX invMat = XMMatrixTranspose(cBufferModelCPU->worldITMatrix);
	t = XMVector3TransformCoord(t, invMat);
	x = DirectX::XMVectorGetX(t);
	z = DirectX::XMVectorGetZ(t);
	// calculate height in terrain model coordinates
	x /= (float)(width); z /= (float)(height);
	x *= (COLOUR_MAP_SIZE - 1); z *= (COLOUR_MAP_SIZE - 1);
	if (x > 0.0f && x < (float)COLOUR_MAP_SIZE && z>0.0f && z < (float)COLOUR_MAP_SIZE)
	{
		DirectX::XMUINT4 col = DirectX::XMUINT4(colour[(int)x][(int)z][0], colour[(int)x][(int)z][1], colour[(int)x][(int)z][2], colour[(int)x][(int)z][3]);
		return col;
	}
	else
		return DirectX::XMUINT4(1.0, 1.0, 1.0, 1.0);

};

void Terrain::setColourMap(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* tex_colour)
{
	D3D11_TEXTURE2D_DESC desc;
	tex_colour->GetDesc(&desc);
	D3D11_TEXTURE2D_DESC StagedDesc = {
		desc.Width,//UINT Width;
		 desc.Height,//UINT Height;
		1,//UINT MipLevels;
		1,//UINT ArraySize;
		DXGI_FORMAT_R8G8B8A8_UNORM,//DXGI_FORMAT Format;
		1, 0,//DXGI_SAMPLE_DESC SampleDesc;
		D3D11_USAGE_STAGING,//D3D11_USAGE Usage;
		0,//UINT BindFlags;
		D3D11_CPU_ACCESS_READ,//UINT CPUAccessFlags;
		0//UINT MiscFlags;
	};


		float incx = desc.Width / (float)COLOUR_MAP_SIZE;
		float incy = desc.Height / (float)COLOUR_MAP_SIZE;
		D3D11_MAPPED_SUBRESOURCE mapColour;
		ID3D11Texture2D* terrainColourStage = 0;
		device->CreateTexture2D(&StagedDesc, NULL, &terrainColourStage);
		context->CopyResource(terrainColourStage, static_cast<ID3D11Resource*>(tex_colour));
		context->Map(terrainColourStage, 0, D3D11_MAP_READ, 0, &mapColour);
		UINT8* Result;
		Result = (UINT8*)mapColour.pData;
		for (float i = 0; i < COLOUR_MAP_SIZE; i++)
			for (float j = 0; j < COLOUR_MAP_SIZE; j++)
				for (int c = 0; c < 4; c++)
					colour[(int)i][(int)j][c] = Result[(int)(i * desc.Width * 4 *incy)+ (int)(j*incx * 4) + c];
				
		context->Unmap(terrainColourStage, 0);
		terrainColourStage->Release();
};

void Terrain::getValueFromHeightMap(UINT8* Result, int i, int j, int width, int height, int texWidth, int texHeight, XMFLOAT3* P, XMFLOAT2 *T)
{
	double incW = ((double)width / ((double)width - 1.0));
	double incH = ((double)height / ((double)height - 1.0));
	double incTW = ((double)1.0 / ((double)width - 1.0));
	double incTH = ((double)1.0 / ((double)height - 1.0));
	P->x = j * incW;
	P->z = i * incH;

	T->x = j * incTW;
	T->y = i * incTH;

	int xi = (int)(T->x * (texWidth - 1));
	int zi = (int)(T->y * (texHeight - 1));
	P->y = (((float)Result[xi * texWidth * 4 + zi * 4]) / 255.0) ;
}



HRESULT Terrain::init(ID3D11Device *device, ID3D11DeviceContext* context, int _width, int _height, ID3D11Texture2D*tex_height)
{
	width = _width;
	height = _height;

	Material material;
	XMCOLOR specular = XMCOLOR(1.0f, 1.0f, 1.0, 1.0);
	XMCOLOR diffuse = XMCOLOR(1.0f, 1.0f, 1.0, 1.0);

	if (instances[0].materials[0])
	{
		shared_ptr<Material>material = instances[0].materials[0];
		diffuse = XMCOLOR(material->getMaterialProperties()->diffuse.x, material->getMaterialProperties()->diffuse.y, material->getMaterialProperties()->diffuse.z, material->getMaterialProperties()->diffuse.w);
		specular = XMCOLOR(material->getMaterialProperties()->specular.x, material->getMaterialProperties()->specular.y, material->getMaterialProperties()->specular.z, material->getMaterialProperties()->specular.w);
	}

	if (vertexBuffer)
		vertexBuffer->Release();
	if (indexBuffer)
		indexBuffer->Release();

	ID3D11Texture2D* grassHeightStage = 0;
	D3D11_TEXTURE2D_DESC heightDesc;
	tex_height->GetDesc(&heightDesc);
	UINT texWidth = heightDesc.Width;
	UINT texHeight = heightDesc.Height;
	cout << "image w" << texWidth << endl;
	// CPU Access buffer
	D3D11_TEXTURE2D_DESC StagedDesc = {
		texWidth,//UINT Width;
		texHeight,//UINT Height;
		1,//UINT MipLevels;
		1,//UINT ArraySize;
		DXGI_FORMAT_R8G8B8A8_UNORM,//DXGI_FORMAT Format;
		1, 0,//DXGI_SAMPLE_DESC SampleDesc;
		D3D11_USAGE_STAGING,//D3D11_USAGE Usage;
		0,//UINT BindFlags;
		D3D11_CPU_ACCESS_READ,//UINT CPUAccessFlags;
		0//UINT MiscFlags;
	};
	device->CreateTexture2D(&StagedDesc, NULL, &grassHeightStage);
	context->CopyResource(grassHeightStage, static_cast<ID3D11Resource*>(tex_height));

	// Lock the memory
	D3D11_MAPPED_SUBRESOURCE MappingDescHeight;
	context->Map(grassHeightStage, 0, D3D11_MAP_READ, 0, &MappingDescHeight);


	// Get the data
	if (MappingDescHeight.pData == NULL) {
		cout<< "DrawSurface_GetPixelColor: Could not read the pixel color because the mapped subresource returned NULL."<<endl;
	}
	else {

		UINT8* Result;
		Result = (UINT8*)MappingDescHeight.pData;

		//INITIALISE Verticies
		vertices = (ExtendedVertexStruct*)malloc(sizeof(ExtendedVertexStruct)*width*height);
		numInd = ((width - 1) * 2 * 3)*(height - 1);
		indices = (UINT*)malloc(sizeof(UINT)*numInd);
		double incW = ((double)width / ((double)width - 1.0));
		double incH = ((double)height / ((double)height - 1.0));
		double incTW = ((double)1.0/ ((double)width - 1.0));
		double incTH = ((double)1.0 / ((double)height - 1.0));
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				XMFLOAT3 P, L, R, T, B;
				XMFLOAT2 Coords;
				getValueFromHeightMap(Result, i, max(j - 1, 0), width, height, texWidth, texHeight, &L, &Coords);
				getValueFromHeightMap(Result, i, min(j + 1, width-1), width, height, texWidth, texHeight, &R, &Coords);
				getValueFromHeightMap(Result, max(i - 1, 0), j, width, height, texWidth, texHeight, &B, &Coords);
				getValueFromHeightMap(Result, min(i + 1, height - 1), j, width, height, texWidth, texHeight, &T, &Coords);
				getValueFromHeightMap(Result, i, j, width, height, texWidth, texHeight, &P, &Coords);

				vertices[(i * width) + j].pos = P;
				vertices[(i * width) + j].texCoord = Coords;
				XMFLOAT3 N = XMFLOAT3((L.y - R.y)/2.0f,1.0f, (T.y - B.y)/2.0f);
				vertices[(i * width) + j].normal = N;
				vertices[(i * width) + j].tangent.x = 0;
				vertices[(i * width) + j].tangent.y = -N.z;
				vertices[(i * width) + j].tangent.z = N.y;
				vertices[(i*width) + j].matDiffuse = diffuse;
				vertices[(i*width) + j].matSpecular = specular;
			}
		}

		//Copy the matrices into the  vertex buffer
		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexDesc.ByteWidth = sizeof(ExtendedVertexStruct)* width*height;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexData.pSysMem = vertices;

		HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);

		// Unlock the memory
		context->Unmap(grassHeightStage, 0);

		for (int i = 0; i < height - 1; i++)
		{
			for (int j = 0; j < width - 1; j++)
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
	}

	// If textures are used a sampler is required for the pixel shader to sample the texture
	D3D11_SAMPLER_DESC linearDesc;

	ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
	linearDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	linearDesc.MaxAnisotropy = 16;
	linearDesc.MinLOD = 0.0f;
	linearDesc.MaxLOD = D3D11_FLOAT32_MAX;
	linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

	device->CreateSamplerState(&linearDesc, &anisoSampler);

	// Dispose of local resources
	grassHeightStage->Release();
	//tex_height->Release();
	//grassNormalStage->Release();
	if (indices)
		free(indices);


	return S_OK;
} 
float Terrain::CalculateYValueWorld(float x, float z)
{
	// transform input from world coordinates to terrain model coordinates
	DirectX::XMVECTOR t = XMLoadFloat3(&XMFLOAT3(x, 0, z));
	XMMATRIX invMat = XMMatrixTranspose(cBufferModelCPU->worldITMatrix);
	t = XMVector3TransformCoord(t, invMat);
	x = XMVectorGetX(t);
	z = XMVectorGetZ(t);

	// calculate height in terrain model coordinates
	float y = CalculateYValue(x / width, z / height);
	// transform height back to world coordinates
	t = XMVectorSetY(t,y);
	t = XMVector3TransformCoord(t, cBufferModelCPU->worldMatrix);
	y = XMVectorGetY(t);

	return y;
}

float Terrain::CalculateYValue(float x, float z)
{

	x = x*width;
	z = z*height;
	//	check range
	if (x<0 || x>this->width || z<0 || z>this->height)
		return 0;

	//	Retrieve the points for the current quad we are in
	float fTopLeft = vertices[(int)x + (((int)z + 1) * this->width)].pos.y;
	float fTopRight = vertices[((int)x) + (((int)z + 1) * this->width) + 1].pos.y;
	float fBottomLeft = vertices[((int)x) + (((int)z) * this->width)].pos.y;
	float fBottomRight = vertices[((int)x) + (((int)z) * this->width) + 1].pos.y;

	float finalHeight = 0;	//	what we are trying to find
	//	fraction parts of x and z
	float frac_x = x - ((int)x);
	float frac_z = z - ((int)z);


	//	What triangle are we in? (top right or bottom left)
	if ((frac_x*frac_x + frac_z*frac_z) < ((1 - frac_x)*(1 - frac_x) + (1 - frac_z)*(1 - frac_z)))
	{
		//in bottom left triangle
		float dX = fBottomRight - fBottomLeft;	//	how the height changes with respect to x
		float dZ = fTopLeft - fBottomLeft;		//	how the height changes with respect to z
		//	calculate our final height
		finalHeight = dX * frac_x + dZ * frac_z + fBottomLeft;
	}
	else
	{
		//in top right triangle
		float dX = fTopLeft - fTopRight;	//	how the height changes with respect to x
		float dZ = fBottomRight - fTopRight;//	how the height changes with respect to z
		//	calculate our final height
		finalHeight = dX * (1 - frac_x) + dZ * (1 - frac_z) + fTopRight;
	}
	//	return our calculated height
	return ((float)finalHeight);
}

Terrain::~Terrain()
{
	if (vertices)
		free(vertices);

	if (anisoSampler)
		anisoSampler->Release();
}


void Terrain::render(ID3D11DeviceContext *context, int instanceIndex ) {

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


	if (anisoSampler) {
		context->PSSetSamplers(0, 1, &anisoSampler);
	}

	// Set Model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ExtendedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw Mesh
	context->DrawIndexed(numInd, 0, 0);
}

