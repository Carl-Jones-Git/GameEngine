
#include "pch.h"
#include "GPUParticles.h"
#include "VertexStructures.h"
#include <iostream>
#include <exception>
#include "Effect.h"

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;





HRESULT GPUParticles::init(ID3D11Device *device, Effect *_updateEffect) {
	
	//effect = _effect;
	updateEffect = _updateEffect;
	//material = _material;
	//inputLayout = effect->getVSInputLayout();
	//

	try
	{
		//INITIALISE Verticies

		for (int i = 0; i<(N_PART*4); i ++)
		{

			vertices[i + 0].velocity = XMFLOAT3(((FLOAT)rand() / RAND_MAX) - 0.5, ((FLOAT)rand() / RAND_MAX)*0.5+0.5, ((FLOAT)rand() / RAND_MAX) - 0.5);
			//No simple hlsl rand() so store the original velocity to respawn dead particles
			//vertices[i + 0].posL = XMFLOAT3(vertices[i + 0].velocity);//store original velocity
			vertices[i + 0].posL = XMFLOAT3(vertices[i + 0].velocity.x, vertices[i + 0].velocity.y, vertices[i + 0].velocity.z);//store original velocity
			vertices[i + 0].data = XMFLOAT3(((FLOAT)rand() / RAND_MAX)*1.2 - 1.2, 0.0f, 0.0f);
			//vertices[i + 0].data = XMFLOAT3(((FLOAT)rand() / RAND_MAX), 0.0f, 0.0f);
			vertices[i + 0].pos = XMFLOAT3(0, 0, 0);// XMFLOAT3(vertices[i + 0].velocity.x*vertices[i + 0].data.x, vertices[i + 0].velocity.y*vertices[i + 0].data.x, vertices[i + 0].velocity.z*vertices[i + 0].data.x);// XMFLOAT3(0, 0, 0);// 
		}

		// Setup particles vertex buffer

		if (!device || !inputLayout)
			throw exception("Invalid parameters for particles instantiation");

		// Setup vertex buffer
		D3D11_BUFFER_DESC vertexDesc;
		D3D11_SUBRESOURCE_DATA vertexdata;

		ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
		ZeroMemory(&vertexdata, sizeof(D3D11_SUBRESOURCE_DATA));

		vertexDesc.Usage = D3D11_USAGE_DEFAULT;
		vertexDesc.ByteWidth = sizeof(ParticleVertexStruct) * N_PART*4;
		vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER|D3D11_BIND_STREAM_OUTPUT;;
		vertexdata.pSysMem = vertices;

		HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexdata, &vertexBuffer);

		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer cannot be created");


		hr = device->CreateBuffer(&vertexDesc, &vertexdata, &vertexBuffer2);

		if (!SUCCEEDED(hr))
			throw exception("Vertex buffer2 cannot be created");

		//textureResourceView = tex_view;

		//if (textureResourceView)
		//	textureResourceView->AddRef(); // We didnt create it here but dont want it deleted by the creator untill we have deconstructed

		//D3D11_SAMPLER_DESC samplerDesc;

		//ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

		//samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

		//samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		//samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		//samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		//samplerDesc.MaxAnisotropy=16;
		//samplerDesc.MinLOD = 0.0f;
		//samplerDesc.MaxLOD = 0.0f;
		//samplerDesc.MipLODBias = 0.0f;
		//samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

		//hr = device->CreateSamplerState(&samplerDesc, &linearSampler);
	}
	catch (exception& e)
	{
		cout << "Particles object could not be instantiated due to:\n";
		cout << e.what() << endl;

		if (vertexBuffer)
			vertexBuffer->Release();

		if (inputLayout)
			inputLayout->Release();

		vertexBuffer = nullptr;
		inputLayout = nullptr;
		indexBuffer = nullptr;
	}
}


void GPUParticles::render(ID3D11DeviceContext *context) {

	// Validate object before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !effect)
		return;

	effect->bindPipeline(context);

	// Set vertex layout
	context->IASetInputLayout(effect->getVSInputLayout());

	// Set vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer};
	UINT vertexStrides[] = { sizeof(ParticleVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (numTextures>0 && sampler) {

		context->PSSetShaderResources(0, numTextures, textures);
		context->PSSetSamplers(0, 1, &sampler);
	}

	// Draw particles object using Geometry Shader
	context->Draw(N_PART*4, 0);

	// release buffers from to SO IA
	vertexBuffers[0] = NULL;
	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
}
void GPUParticles::setWorldMatrix(XMMATRIX _worldMatrix) {
	cBufferModelCPU->worldMatrix = _worldMatrix;
	XMVECTOR det = XMMatrixDeterminant(_worldMatrix);
	//cBufferModelCPU->worldITMatrix = XMMatrixInverse(&det, XMMatrixTranspose(_worldMatrix));
	XMVECTOR scale,rotQuat,trans;
	XMMatrixDecompose(&scale, &rotQuat, &trans, _worldMatrix);
	XMMATRIX temp = XMMatrixRotationQuaternion(rotQuat);
	cBufferModelCPU->worldITMatrix = XMMatrixInverse(&det, temp);
}

void GPUParticles::updateParticles(ID3D11DeviceContext *context) {
	
	// Validate object before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !vertexBuffer2 || !updateEffect)
		return;

	updateEffect->bindPipeline(context);

	// Set vertex layout
	context->IASetInputLayout(updateEffect->getVSInputLayout());

	// Set vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ParticleVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	// bind result buffer to SO
	vertexBuffers[0] =  vertexBuffer2 ;
	context->SOSetTargets(1, vertexBuffers, vertexOffsets);


	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (numTextures>0 && sampler) {

		context->PSSetShaderResources(0, numTextures, textures);
		context->PSSetSamplers(0, 1, &sampler);
	}

	context->PSSetShader(NULL,0, 0);

	// Draw particles object using Geometry Shader only
	context->Draw(N_PART*4, 0);
	
	// release buffers from to SO IA
	vertexBuffers[0] = NULL;
	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->SOSetTargets(1, vertexBuffers, vertexOffsets);
	//swap buffers
	ID3D11Buffer *bufferTmp = vertexBuffer;
	vertexBuffer = vertexBuffer2;
	vertexBuffer2 = bufferTmp;
}
