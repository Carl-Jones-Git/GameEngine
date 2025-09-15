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
#include "SkinnedModel.h"
#include "Material.h"
#include "Effect.h"
#include <iostream>
#include <exception>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

XMMATRIX AIMat2XMMat(aiMatrix4x4 m)
{
	return XMMATRIX(m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2, m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4);
}
XMMATRIX AIMat2XMMat(aiMatrix3x3 m)
{
	XMFLOAT3X3 temp = XMFLOAT3X3(m.a1, m.b1, m.c1, m.a2, m.b2, m.c2, m.a3, m.b3, m.c3);
	return XMLoadFloat3x3(&temp);
}

void SkinnedModel::loadBones(ID3D11Device *device) {

	XMMATRIX rootTransform = AIMat2XMMat(scene->mRootNode->mTransformation);
	invGlobalTransform = XMMatrixInverse(NULL, rootTransform);
	setWorldMatrix(invGlobalTransform);

	SkinnedVertexStruct *_vertexBuffer = (SkinnedVertexStruct*)malloc(num_vert * sizeof(SkinnedVertexStruct));

	for (int i = 0; i < num_vert; i++)
	{
		_vertexBuffer[i].pos = vertexBufferCPU[i].pos;
		_vertexBuffer[i].normal = vertexBufferCPU[i].normal;
		_vertexBuffer[i].tangent = vertexBufferCPU[i].tangent;
		_vertexBuffer[i].texCoord = vertexBufferCPU[i].texCoord;
		_vertexBuffer[i].matDiffuse = vertexBufferCPU[i].matDiffuse;
		_vertexBuffer[i].matSpecular = vertexBufferCPU[i].matSpecular;
		for(int j=0;j<12;j++)
			_vertexBuffer[i].boneWeights[j] = 0;
		for (int j = 0; j < 12; j++)
			_vertexBuffer[i].boneIndices[j] = -1;
	}

	int max_num_weights = 0;
	
	numBones = 0;
	for (int k = 0; k < scene->mNumMeshes; k++)
		numBones += scene->mMeshes[k]->mNumBones;
	
	bones = (Bone*)malloc(sizeof(Bone) * numBones);
	
	int boneCounter = 0;
	for (int k = 0; k < scene->mNumMeshes; k++)
	{
		aiMesh* mesh = scene->mMeshes[k];

		for (int i = 0; i < mesh->mNumBones; i++)
		{
			string boneName(mesh->mBones[i]->mName.data);
			int BoneIndex = 0;
			if (m_BoneMapping.find(boneName) == m_BoneMapping.end()) {
				BoneIndex = boneCounter;
				boneCounter++;
			}
			else {
				BoneIndex = m_BoneMapping[boneName];
			}
			int max_vertID = 0;
			
			bones[BoneIndex].offMatrix = AIMat2XMMat(mesh->mBones[i]->mOffsetMatrix);
			m_BoneMapping[boneName] = BoneIndex;
			for (int j = 0; j < mesh->mBones[i]->mNumWeights; j++)
			{
				int index = baseVertexOffset[k] + mesh->mBones[i]->mWeights[j].mVertexId;

				if (index > max_vertID)
					max_vertID = index;
				if (index >= 0)
				{
					bool overflow = true;
					for (int l = 0; l < 12; l++)
						if (_vertexBuffer[index].boneIndices[l] < 0)
						{
							_vertexBuffer[index].boneWeights[l] = mesh->mBones[i]->mWeights[j].mWeight;
							_vertexBuffer[index].boneIndices[l] = BoneIndex;

							if (max_num_weights < l+1) 
								max_num_weights = l+1;
							overflow = false;
							break;
						}
					if (overflow)
						std::cout << "too many weights" << endl;
				}
			}

		}
	}
	numBones = boneCounter;
	float maxsumofweights = 0;
	float minsumofweights = 10;

	//normalise weights 
	for (int i = 0; i < num_vert; i++)
	{
		float boneweightsum = 0;

		for (int j = 0; j < 12; j++)
			boneweightsum += _vertexBuffer[i].boneWeights[j];
		for (int j = 0; j < 12; j++)
			_vertexBuffer[i].boneWeights[j] /= boneweightsum;
	}

	
	if(vertexBuffer)
		vertexBuffer->Release();

	// Setup DX vertex buffer interfaces
	D3D11_BUFFER_DESC vertexDesc;
	D3D11_SUBRESOURCE_DATA vertexData;

	ZeroMemory(&vertexDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));

	vertexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.ByteWidth = num_vert * sizeof(SkinnedVertexStruct);
	vertexData.pSysMem = _vertexBuffer;

	HRESULT hr = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBuffer);
	cBufferBonesCPU = (CBufferBone*)_aligned_malloc(sizeof(CBufferBone)*numBones,16);
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferBone)*numBones;
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferBonesCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	 hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,&cBufferBonesGPU);
	 free(_vertexBuffer);
}


double SkinnedModel::getAnimTime(float timeS)
{
	float ticksPerSecond = scene->mAnimations[currentAnim]->mTicksPerSecond != 0 ? scene->mAnimations[currentAnim]->mTicksPerSecond : 25.0f;
	float ticks = timeS * ticksPerSecond;
	float animTime = fmod(ticks, scene->mAnimations[currentAnim]->mDuration);
	return animTime/ ticksPerSecond;
}

double SkinnedModel::getAnimLength()
{
	float ticksPerSecond = scene->mAnimations[currentAnim]->mTicksPerSecond != 0 ? scene->mAnimations[currentAnim]->mTicksPerSecond : 25.0f;
	return scene->mAnimations[currentAnim]->mDuration / ticksPerSecond;
}

XMMATRIX SkinnedModel::boneTransform(float timeS,int anim)
{
	XMMATRIX ident=XMMatrixIdentity();
	if (anim >= scene->mNumAnimations)anim = scene->mNumAnimations - 1;
	float ticksPerSecond = scene->mAnimations[anim]->mTicksPerSecond != 0 ?scene->mAnimations[anim]->mTicksPerSecond : 25.0f;
	float ticks = timeS * ticksPerSecond;
	float animTime = fmod(ticks, scene->mAnimations[anim]->mDuration);
	
	readNodeHeirarchy(animTime, scene->mRootNode, ident, anim);

	for (int i = 0; i < numBones; i++) {
		cBufferBonesCPU[i].boneMatrix = bones[i].finalTrans;
	}

	return XMMatrixIdentity();
}

const aiNodeAnim* SkinnedModel::findNodeAnim(const aiAnimation* pAnimation, const aiString* NodeName)
{
	for (int i = 0; i < pAnimation->mNumChannels; i++) {
		const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

		if (pNodeAnim->mNodeName == *NodeName) {
			return pNodeAnim;
		}
	}

	return NULL;
}

void SkinnedModel::readNodeHeirarchy(float animTime, const aiNode* node, const XMMATRIX parentTransform,int anim)
{
	string nodeName(node->mName.data);
	const aiAnimation* animation = scene->mAnimations[anim];
	XMMATRIX nodeTrans = AIMat2XMMat(node->mTransformation);
	const aiNodeAnim* nodeAnim = findNodeAnim(animation, &(node->mName));


	if (nodeAnim) {
		// Interpolate scaling and generate scaling transformation matrix
		aiVector3D scaling;
		calcInterpolatedScaling(scaling, animTime, nodeAnim);
		XMMATRIX scalingMatrix = XMMatrixScaling(scaling.x, scaling.y, scaling.z);

		// Interpolate rotation and generate rotation transformation matrix
		aiQuaternion rotation;
		calcInterpolatedRotation(rotation, animTime, nodeAnim);
		aiMatrix3x3 m = rotation.GetMatrix();
		XMMATRIX rotMatrix = AIMat2XMMat(m);// XMLoadFloat3x3(&temp);

		// Interpolate translation and generate translation transformation matrix
		aiVector3D trans;
		calcInterpolatedPosition(trans, animTime, nodeAnim);
		XMMATRIX transMatrix = XMMatrixTranslation(trans.x, trans.y, trans.z);

		// Combine the above transformations
		nodeTrans = rotMatrix*transMatrix*scalingMatrix;
	}

	XMMATRIX globalTransformation  = nodeTrans*parentTransform;

	if (m_BoneMapping.find(nodeName) != m_BoneMapping.end()) {
		int boneIndex = m_BoneMapping[nodeName];
		bones[boneIndex].finalTrans = bones[boneIndex].offMatrix*globalTransformation*invGlobalTransform;
	}
	
	for (int i = 0; i < node->mNumChildren; i++) {
		readNodeHeirarchy(animTime, node->mChildren[i], globalTransformation,anim);
	}
}

int SkinnedModel::findPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	for (int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}

	return pNodeAnim->mNumPositionKeys - 2;
	
	assert(0);
	return 0;
}


int SkinnedModel::findRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumRotationKeys > 0);

	for (int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}

	return pNodeAnim->mNumRotationKeys - 2;

	assert(0);
	return 0;
}


int SkinnedModel::findScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumScalingKeys > 0);

	for (int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}
	return pNodeAnim->mNumScalingKeys - 2;
	assert(0);
	return 0;
}


void SkinnedModel::calcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1].mTime < AnimationTime) {
		Out = pNodeAnim->mPositionKeys[pNodeAnim->mNumPositionKeys - 1].mValue;
		return;
	}
	int PositionIndex = findPosition(AnimationTime, pNodeAnim);
	int NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
	float Factor = 0.0f;
	if (DeltaTime > 0.0f)Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	
	const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}


void SkinnedModel::calcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	if (pNodeAnim->mRotationKeys[pNodeAnim->mNumRotationKeys - 1].mTime < AnimationTime) {
		Out = pNodeAnim->mRotationKeys[pNodeAnim->mNumRotationKeys - 1].mValue;
		return;
	}


	int RotationIndex = findRotation(AnimationTime, pNodeAnim);
	int NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
	float Factor = 0.0f;
	if (DeltaTime > 0.0f)Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
	Out = Out.Normalize();
}


void SkinnedModel::calcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{

	if (pNodeAnim->mScalingKeys[pNodeAnim->mNumScalingKeys-1].mTime < AnimationTime) {
		Out = pNodeAnim->mScalingKeys[pNodeAnim->mNumScalingKeys - 1].mValue;
		return;
	}

	int ScalingIndex = findScaling(AnimationTime, pNodeAnim);
	int NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
	float Factor = 0.0f;
	if(DeltaTime>0.0f)Factor=(AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	aiVector3D Delta = End - Start;
	Out = Start + Factor * Delta;
}

//
void SkinnedModel::update(ID3D11DeviceContext *context) {
	BaseModel::update(context); //call baseModel update
	mapCbuffer(context, cBufferBonesCPU, cBufferBonesGPU,sizeof(CBufferBone)*numBones);
}

void SkinnedModel::updateBones(double time)
{
	boneTransform(time, currentAnim);
}


void SkinnedModel::render(ID3D11DeviceContext *context, int instanceIndex) {//, int mode
	effect->bindPipeline(context);

	// Apply the cBuffer.
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->VSSetConstantBuffers(6, 1, &cBufferBonesGPU);
	// Validate Model before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !indexBuffer || !effect)
		return;

	// Set vertex layout
	context->IASetInputLayout(effect->getVSInputLayout());

	// Set Model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(SkinnedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (sampler) {
		context->PSSetSamplers(0, 1, &sampler);
	}



	// Draw Model
	for (uint32_t indexOffset = 0, i = 0; i < numMeshes; indexOffset += indexCount[i], ++i)
	{
		if (instances[instanceIndex].materials.size() > i)
			instances[instanceIndex].materials[i]->update(context);
		context->DrawIndexed(indexCount[i], indexOffset, baseVertexOffset[i]);
	}
}

void SkinnedModel::renderSimp(ID3D11DeviceContext *context) {

	// Validate Model before rendering (see notes in constructor)
	if (!context || !vertexBuffer || !indexBuffer )
		return;

	// Set vertex layout
	context->IASetInputLayout(effect->getVSInputLayout());

	// Set Model vertex and index buffers for IA
	ID3D11Buffer* vertexBuffers[] = { vertexBuffer };
	UINT vertexStrides[] = { sizeof(ExtendedVertexStruct) };
	UINT vertexOffsets[] = { 0 };

	context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, vertexOffsets);
	context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	// Set primitive topology for IA
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// Bind texture resource views and texture sampler objects to the PS stage of the pipeline
	if (sampler) {
		context->PSSetSamplers(0, 1, &sampler);
	}

	// Apply the cBuffer.
	context->VSSetConstantBuffers(0, 1, &cBufferModelGPU);
	context->PSSetConstantBuffers(0, 1, &cBufferModelGPU);

	// Draw Model
	for (uint32_t indexOffset = 0, i = 0; i < numMeshes; indexOffset += indexCount[i], ++i)
		context->DrawIndexed(indexCount[i], indexOffset, baseVertexOffset[i]);
}

SkinnedModel::~SkinnedModel() {
	if (bones)
		free(bones);
	if (cBufferBonesCPU)
		_aligned_free(cBufferBonesCPU);
	if (cBufferBonesGPU)
		cBufferBonesGPU->Release();
}