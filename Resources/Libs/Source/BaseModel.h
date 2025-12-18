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
#include <wrl/client.h>
#include "Effect.h"
#include <Material.h>
#include <Texture.h>
#include <memory>
#include <vector>

#include <PxPhysicsAPI.h>
#include <PxFiltering.h>

using Microsoft::WRL::ComPtr;

constexpr int MAX_MATERIALS = 8;

struct Instance
{
	physx::PxRigidDynamic* dynamicPX = nullptr;
	DirectX::XMMATRIX worldMatrix;
	std::vector<std::shared_ptr<Material>> materials;

	explicit Instance(DirectX::XMMATRIX _worldMatrix, std::shared_ptr<Material> _material = nullptr)
		: worldMatrix(_worldMatrix)
	{
		if (_material) {
			materials.push_back(std::move(_material));
		}
	}
	//Instance(DirectX::XMMATRIX _worldMatrix, std::shared_ptr < Material > _material) {
	//	worldMatrix = _worldMatrix;
	//	if (_material) materials.push_back(_material);
	//}
	//Instance(DirectX::XMMATRIX _worldMatrix, vector<std::shared_ptr < Material >> _materials) {
	//	worldMatrix = _worldMatrix;
	//	for (int i = 0; i < _materials.size(); i++)
	//		materials.push_back(_materials[i]);
	//}


	Instance(DirectX::XMMATRIX _worldMatrix, std::vector<std::shared_ptr<Material>> _materials)
		: worldMatrix(_worldMatrix), materials(std::move(_materials))
	{
	}
};

// Abstract base class to model mesh objects for rendering in DirectX
class BaseModel {
public:
	BaseModel(ID3D11Device* device, std::shared_ptr<Effect> _effect,
		std::shared_ptr<Material> _material = nullptr, int _meshNumber = -1);
	virtual ~BaseModel();

	// Disable copy operations
	BaseModel(const BaseModel&) = delete;
	BaseModel& operator=(const BaseModel&) = delete;

	// Enable move operations
	BaseModel(BaseModel&&) noexcept = default;
	BaseModel& operator=(BaseModel&&) noexcept = default;

	virtual void render(ID3D11DeviceContext* context, int instanceIndex = 0) = 0;
	virtual HRESULT init(ID3D11Device* device) = 0;

	void update(ID3D11DeviceContext* context, int i = 0);
	void renderInstances(ID3D11DeviceContext* context);

	// Material management
	void setMaterial(ID3D11Device* device, std::shared_ptr<Material> _material,
		int instanceIndex = 0, int materialIndex = 0);
	std::shared_ptr<Material> getMaterial(int instanceIndex = 0, int materialIndex = 0) const { return instances[instanceIndex].materials[materialIndex]; };
	vector<std::shared_ptr < Material > >getMaterials(int instanceIndex = 0) const { return instances[instanceIndex].materials; }


	// Effect management
	void setEffect(std::shared_ptr<Effect> _effect) { effect = std::move(_effect); }
	std::shared_ptr<Effect> getEffect() const { return effect; }

	// World matrix management
	void setWorldMatrix(DirectX::XMMATRIX _worldMatrix, int n = 0);
	DirectX::XMMATRIX getWorldMatrix(int n = 0) const { return instances[n].worldMatrix; };

	// Visibility
	void setVisible(bool vis) { visible = vis; }
	bool getVisible() const { return visible; }

	// Sampler management
	void setSampler(ID3D11SamplerState* _sampler) { if (sampler)sampler->Release(); sampler = _sampler; };

	// Instance management
	void setDynamicPX(int _instance, physx::PxRigidDynamic* _dynamicPX = nullptr) { instances[_instance].dynamicPX = _dynamicPX; };
	int getNumInstances() { return instances.size(); };
	void addInstance(DirectX::XMMATRIX _worldMatrix, std::shared_ptr < Material > _material) { instances.push_back(Instance(_worldMatrix, _material)); };
	void addInstance(DirectX::XMMATRIX _worldMatrix, vector<std::shared_ptr < Material >> _materials) { instances.push_back(Instance(_worldMatrix, _materials)); };

protected:
	void initCBuffer(ID3D11Device* device);
	void createDefaultLinearSampler(ID3D11Device* device);
	std::vector<Instance> instances;
	int meshNumber;
	ComPtr<ID3D11Buffer> vertexBuffer;
	ComPtr<ID3D11Buffer> indexBuffer;
	std::shared_ptr<Effect> effect;
	ComPtr<ID3D11SamplerState> sampler;
	std::unique_ptr<CBufferModel, decltype(&_aligned_free)> cBufferModelCPU{ nullptr, &_aligned_free };
	ComPtr<ID3D11Buffer> cBufferModelGPU;

	bool visible = true;
};