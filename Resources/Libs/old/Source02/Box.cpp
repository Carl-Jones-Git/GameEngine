
#include <stdafx.h>
#include <Box.h>
#include <VertexStructures.h>
#include <iostream>
#include <exception>




HRESULT Box::init(ID3D11Device *device) {
	Material material;
	XMFLOAT2 emptyCoord = XMFLOAT2(0.0f, 0.0f);
	XMCOLOR specular = XMCOLOR(1.0f, 1.0f, 1.0, 1.0);
	XMCOLOR diffuse = XMCOLOR(1.0f, 1.0f, 1.0, 1.0);

	if (instances[0].materials.size()>0)
	{
		shared_ptr<Material>material = instances[0].materials[0];
		diffuse = XMCOLOR(material->getColour()->diffuse.x, material->getColour()->diffuse.y, material->getColour()->diffuse.z, material->getColour()->diffuse.w);
		specular = XMCOLOR(material->getColour()->specular.x, material->getColour()->specular.y, material->getColour()->specular.z, material->getColour()->specular.w);
	}
		ExtendedVertexStruct vertices[] = {

			//Front face
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT3(0.0f, 0.0f, -1.0f),diffuse, specular, XMFLOAT2(0.0f, 0.0f) }, //0
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT3(0.0f, 0.0f, -1.0f),diffuse,specular, XMFLOAT2(0.0f, +1.0f) }, //1
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT3(0.0f, 0.0f, -1.0f),diffuse,specular, XMFLOAT2(+1.0f, +1.0f) }, //2
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),XMFLOAT3(0.0f, 0.0f, -1.0f),diffuse,specular, XMFLOAT2(+1.0f, 0.0f) }, //3

		//Back face
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f),diffuse,specular, XMFLOAT2(0.0f, 0.0f) }, //4
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f),diffuse,specular, XMFLOAT2(0.0f, +1.0f) }, //5
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f),diffuse,specular, XMFLOAT2(+1.0f, +1.0f) }, //6
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f),diffuse,specular, XMFLOAT2(+1.0f, 0.0f) }, //7

		//Left Face duplicate 4, 5, 1, 4, 1, 0 -> 8,9,10,8,10,11
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT3(-1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, 0.0f) }, //4->8
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT3(-1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, +1.0f) }, //5->9
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT3(-1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, +1.0f) }, //1->10
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),XMFLOAT3(-1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, 0.0f) }, //0->11

		//Right Face duplicate 3, 2, 6, 3, 6, 7, ->12,13,14,12,14,15,
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, 0.0f) },//3->12
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, +1.0f) },//2->13
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, +1.0f) },//6->14
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, 0.0f) }, //7->15

		//Top Face duplicate1, 5, 6, 1, 6, 2 ->16,17,18,16,18,19,
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),XMFLOAT3(0.0f, +1.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, 0.0f) }, //1->16
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),XMFLOAT3(0.0f, +1.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, +1.0f) }, //5->17
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),XMFLOAT3(0.0f, +1.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, +1.0f) }, //6->18
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),XMFLOAT3(0.0f, +1.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, 0.0f) }, //2->19

		//Bottom Face duplicate4, 0, 3, 4, 3, 7 -> 20,21,22,20,22,23,
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT3(0.0f, -1.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, 0.0f) }, //4->20
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT3(0.0f, -1.0f, 0.0f),diffuse,specular, XMFLOAT2(0.0f, +1.0f) }, //0->21
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT3(0.0f, -1.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, +1.0f) }, //3->22
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f),XMFLOAT3(0.0f, -1.0f, 0.0f),diffuse,specular, XMFLOAT2(+1.0f, 0.0f) } //7->23

	}; //end of box definition



	// Create the indices
	UINT indices[] = {

		// front face OK
		0, 1, 2,
		0, 2, 3,

		// back face OK
		4, 6, 5,
		4, 7, 6,

		// left face
		//4, 5, 1,4, 1, 0,
		8, 9, 10,
		8, 10, 11,

		// right face
		//3, 2, 6,3, 6, 7,
		12, 13, 14,
		12, 14, 15,

		// top face
		//1, 5, 6,1, 6, 2,
		16, 17, 18,
		16, 18, 19,

		// bottom face
		//4, 0, 3, 4, 3, 7
		20, 21, 22,
		20, 22, 23,
	};



	try
	{
		// Setup Box vertex buffer
		if (!device )
			throw exception("Invalid parameters for triangle model instantiation");

		// Setup vertex buffer

		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexData;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexDesc.ByteWidth = sizeof(ExtendedVertexStruct) * 24;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexData.pSysMem = vertices;

		HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);
		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer cannot be created");

		// Setup Box index buffer
		D3D11_BUFFER_DESC indexDesc;
		indexDesc.Usage = D3D11_USAGE_IMMUTABLE;
		indexDesc.ByteWidth = sizeof(UINT) * 36;
		indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		indexDesc.CPUAccessFlags = 0;
		indexDesc.MiscFlags = 0;
		indexDesc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA indexData;
		indexData.pSysMem = indices;

		hr = device->CreateBuffer(&indexDesc, &indexData, &indexBuffer);

		if (!SUCCEEDED(hr))
			throw exception("Index buffer cannot be created");
		// If textures are used a sampler is required for the pixel shader to sample the texture
		D3D11_SAMPLER_DESC linearDesc;

		ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
		linearDesc.Filter = D3D11_FILTER_ANISOTROPIC;
		//linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
		linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
		linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
		linearDesc.MaxAnisotropy = 16;

		linearDesc.MinLOD = 0.0f;
		linearDesc.MaxLOD = D3D11_FLOAT32_MAX;
		linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

		device->CreateSamplerState(&linearDesc, &anisoSampler);
	}

	catch (exception& e)
	{
		cout << "Triangle object could not be instantiated due to:\n";
		cout << e.what() << endl;

		if (vertexBuffer)
			vertexBuffer->Release();

		if (indexBuffer)
			indexBuffer->Release();
		indexBuffer = nullptr;
		vertexBuffer = nullptr;
		return E_FAIL;
	}


	return S_OK;
}


void Box::render(ID3D11DeviceContext *context, int instanceIndex) {

	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);

	// Validate object before rendering 
	if (!context || !vertexBuffer)
		return;
	
	if (effect)
		// Sets shaders, states
		effect->bindPipeline(context);
	if(instances[instanceIndex].materials.size()>0)
		instances[instanceIndex].materials[0]->update(context);
	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if ( anisoSampler) {

		//context->PSSetShaderResources(0, numTextures, textures);
		context->PSSetSamplers(0, 1, &anisoSampler);
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
	context->DrawIndexed(36,0,0);
}


Box::~Box() {
	if (vertexBuffer)
		vertexBuffer->Release();
	if (indexBuffer)
		indexBuffer->Release();
	indexBuffer = nullptr;
	vertexBuffer = nullptr;

	if (anisoSampler)
		anisoSampler->Release();
}
