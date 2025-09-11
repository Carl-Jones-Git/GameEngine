#pragma once
#include <d3d11_2.h>
#include <DirectXMath.h>
#include "Model.h"
//#include <Animation.h>
#include <string>
#include <vector>
#include <cstdint>
#include "CBufferStructures.h"
#include "Utils.h"
#include "Camera.h"
#include "VertexStructures.h"
#include <map>
#include <Assimp\include\assimp\Importer.hpp>      // C++ importer interface
#include <Assimp\include\assimp\scene.h>           // Output data structure
#include <Assimp\include\assimp\postprocess.h>     // Post processing flags

using namespace std;

struct Bone{
	DirectX::XMMATRIX		offMatrix;
	DirectX::XMMATRIX		finalTrans;
	DirectX::XMFLOAT4		weights;
	DirectX::XMINT4			vertexID;
};

class SkinnedModel : public Model {
	int						numBones;
	Bone					*bones = nullptr;
	CBufferBone				*cBufferBonesCPU = nullptr;
	ID3D11Buffer			*cBufferBonesGPU = nullptr;
	map<string, int>		m_BoneMapping; // maps a bone name to its index
	XMMATRIX				invGlobalTransform;
	int						currentAnim=0;

	XMMATRIX boneTransform(float time,int anim);
	const aiNodeAnim* findNodeAnim(const aiAnimation* pAnimation, const aiString* NodeName);
	void readNodeHeirarchy(float animTime, const aiNode* node, const XMMATRIX parentTransform,int anim);
	int findPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
	int findScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
	int findRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
	void calcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void calcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
	void calcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);

public:
	void loadBones(ID3D11Device* device);			
	SkinnedModel(ID3D11DeviceContext* context, ID3D11Device *device, std::wstring& filename, shared_ptr<Effect> _effect, shared_ptr<Material> _material = nullptr) : Model(context,device, filename, _effect, _material, NO_PRETRANS)
	{ 
		//loadBones(device); 
	}
	~SkinnedModel();

	void render(ID3D11DeviceContext *context, int instanceIndex = 0);
	void renderSimp(ID3D11DeviceContext *context);
	void update(ID3D11DeviceContext *context);
	void updateBones(double time);
	void updateBonesSubFrames(int from, int to, double time)
	{
		double t = from / scene->mAnimations[currentAnim]->mTicksPerSecond;;
		if (from != to)
		{
			int nFrames = scene->mAnimations[currentAnim]->mDuration;
			//cout << "Frames" << nFrames << endl;
			//double ticksPerS = nathan->getAnimTicksPerSecond(0);
			double  animLengthS = (to - from) / scene->mAnimations[currentAnim]->mTicksPerSecond;
			t+= fmod(time, animLengthS);
		}
		updateBones(t);
	};
	int getNumAnimimations(){ return scene->mNumAnimations; };
	int getCurrentAnim(){ return currentAnim; };
	void setCurrentAnim(int anim){currentAnim=anim; };
	double getAnimTime(float timeS);
	double getAnimLength();
	double getAnimDuration(int anim) {
		double ticksPerSecond = scene->mAnimations[anim]->mTicksPerSecond != 0 ? scene->mAnimations[anim]->mTicksPerSecond : 25.0;
		return scene->mAnimations[anim]->mDuration / ticksPerSecond;
	};
	double getAnimTicksPerSecond(int anim) { return scene->mAnimations[anim]->mTicksPerSecond; };
	int getAnimFrames(int anim) { return scene->mAnimations[anim]->mDuration; };
};
