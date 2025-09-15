
#pragma once

#include <d3d11_2.h>
#include <DirectXMath.h>
#include <Effect.h>
#include <Material.h>
#include <Texture.h>
#include <PxPhysicsAPI.h>
#include <PxFiltering.h>

#define MAX_MATERIALS 8

struct Instance
{
	//std::shared_ptr < Material > material
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
