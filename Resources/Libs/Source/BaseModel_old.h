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
#include <Includes.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include "Effect.h"
#include <Material.h>
#include <Texture.h>

#include <PxPhysicsAPI.h>
#include <PxFiltering.h>

#define MAX_MATERIALS 8

struct Instance
{
	physx::PxRigidDynamic* dynamicPX = nullptr;
	DirectX::XMMATRIX worldMatrix;
	vector<std::shared_ptr < Material > >materials;
	Instance(DirectX::XMMATRIX _worldMatrix, std::shared_ptr < Material > _material) {
		worldMatrix = _worldMatrix;
		if(_material) materials.push_back(_material);
	}
	Instance(DirectX::XMMATRIX _worldMatrix, vector<std::shared_ptr < Material >> _materials) {
		worldMatrix = _worldMatrix;
		for( int i = 0; i<_materials.size();i++)
		 materials.push_back(_materials[i]);
	}

};

// Abstract base class to model mesh objects for rendering in DirectX
class BaseModel {

public:
	int						meshNumber;
	ID3D11Buffer			*vertexBuffer = nullptr;
	ID3D11Buffer			*indexBuffer = nullptr;
	shared_ptr<Effect>		effect = nullptr;
	ID3D11SamplerState		*sampler = nullptr;
	vector <Instance>		instances;
	CBufferModel			*cBufferModelCPU = nullptr;
	ID3D11Buffer			*cBufferModelGPU = nullptr;
	bool					visible = true;

public:

	BaseModel(ID3D11Device* device, shared_ptr<Effect> _effect, std::shared_ptr<Material> _material=nullptr, int _meshNumber = -1);

	~BaseModel();

	virtual void render(ID3D11DeviceContext *context,int instanceIndex = 0) = 0;
	virtual HRESULT init(ID3D11Device *device) = 0;
	void update(ID3D11DeviceContext *context, int i=0);

	void renderInstances(ID3D11DeviceContext* context)
	{
		for (int i = 0; i < instances.size(); i++)
		{
			if (instances[i].dynamicPX)
			{
				physx::PxTransform modelT = instances[i].dynamicPX->getGlobalPose();
				XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(modelT.q.x, modelT.q.y, modelT.q.z, modelT.q.w));
				instances[i].worldMatrix = (XMMatrixRotationQuaternion(quart) * XMMatrixTranslation(modelT.p.x, modelT.p.y + 0.2, modelT.p.z));
			}
			update(context, i);
			render(context, i);
		}
		update(context, 0);
	}

	void setMaterial(ID3D11Device* device, std::shared_ptr<Material>_material, int instanceIndex = 0, int materialIndex = 0);
	std::shared_ptr<Material> getMaterial(int instanceIndex = 0, int materialIndex = 0) {return instances[instanceIndex].materials[materialIndex];};
	vector<std::shared_ptr < Material > >getMaterials(int instanceIndex = 0) { return instances[instanceIndex].materials; }
	void setEffect(shared_ptr<Effect> _effect){ effect = _effect;};// effect must have the same input layout as the model
	shared_ptr<Effect>  getEffect() { return  effect; };
	void initCBuffer(ID3D11Device *device);
	void createDefaultLinearSampler(ID3D11Device *device);
	void setWorldMatrix(DirectX::XMMATRIX _worldMatrix,int n=0);
	DirectX::XMMATRIX getWorldMatrix(int n=0){ return instances[n].worldMatrix; };
	void setVisible(bool vis) { visible = vis; };
	bool getVisible() { return visible; };
	void setSampler(ID3D11SamplerState* _sampler) { if (sampler)sampler->Release();sampler = _sampler; };
};
