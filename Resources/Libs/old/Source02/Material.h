#pragma once
#include <CBufferStructures.h>
#include "Utils.h"

#define MAX_TEXTURES 8
enum matUsage
{
	USE_MATERIAL_ALL =  0,
	DIFFUSE_MAP = 1 << 0,
	NORMAL_MAP = 1 << 1,
	SPECULAR_MAP=1<<2,
	OPACITY_MAP = 1 << 3,
	REFLECTION_MAP = 1 << 4,
	EMISSIVE_MAP = 1 << 5,
	//...
	CHROME = 1 << 8
};
class Material
{
	CBufferMaterial* cBufferMaterialCPU = nullptr;
	ID3D11Buffer* cBufferMaterialGPU = nullptr;
	//Textures -Diffuse, Normap, Specular, Opacity - not yet used
	ID3D11ShaderResourceView* textureSRVArray[MAX_TEXTURES];
	int numTextures = 0;

public:
	Material();
	Material(ID3D11Device* device);
	Material(ID3D11Device* device, XMFLOAT4 _colour, INT use= USE_MATERIAL_ALL);
	Material(ID3D11Device* device, shared_ptr <Material>_material);
	~Material();
	CBufferMaterial *getColour(){ return cBufferMaterialCPU; };
	void setTexture(ID3D11ShaderResourceView* _textureSRV, int index = 0) { 
		if(textureSRVArray[index])
			textureSRVArray[index]->Release();
		textureSRVArray[index] = _textureSRV;
		if (textureSRVArray[index])
			textureSRVArray[index]->AddRef();
		numTextures = max(numTextures, index + 1);
	}


	void setTextures(ID3D11ShaderResourceView* _textures[], int _numTextures = 1) {

		for (int i = 0; i < numTextures; i++)
			if(textureSRVArray[i])
				textureSRVArray[i]->Release();
		
		numTextures = _numTextures;
		for (int i = 0; i < numTextures; i++)
		{
			textureSRVArray[i] = _textures[i];
			if(textureSRVArray[i])
				textureSRVArray[i]->AddRef();
		}
		
	};
	ID3D11ShaderResourceView* getTexture(int i) { return  textureSRVArray[i]; };
	void setEmissive(XMFLOAT4 _emissive){ cBufferMaterialCPU->emissive = _emissive; };
	void setAmbient(XMFLOAT4 _ambient){ cBufferMaterialCPU->ambient = _ambient; };
	void setDiffuse(XMFLOAT4 _diffuse){ cBufferMaterialCPU->diffuse = _diffuse; };
	XMFLOAT4 getDiffuse() { return cBufferMaterialCPU->diffuse; };
	void setSpecular(XMFLOAT4 _specular){ cBufferMaterialCPU->specular = _specular; };
	void setUsage(INT _usage) { cBufferMaterialCPU->useage = _usage; };
	void update(ID3D11DeviceContext* context);
	void init(ID3D11Device* device);
};

