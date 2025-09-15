#pragma once
#include "BaseModel.h"
#include "CBufferStructures.h"
#include "VertexStructures.h"
#include "Camera.h"
class Effect;
class Material;

const int COLOUR_MAP_SIZE=4096;

class Terrain : public BaseModel
{

	int width, height;
	UINT numInd = 0;
	ExtendedVertexStruct *vertices;
	UINT*indices;
	ID3D11SamplerState* anisoSampler=nullptr;
	//ID3D11SamplerState* shadowSampler = nullptr;
	UINT8 colour[COLOUR_MAP_SIZE][COLOUR_MAP_SIZE][4];


public:
	Terrain(ID3D11Device *device, ID3D11DeviceContext*context, int width, int height, ID3D11Texture2D*tex_height, shared_ptr<Effect> _effect, std::shared_ptr<Material> _material = nullptr) : BaseModel(device, _effect, _material){ init(device, context,width,height, tex_height); };
	float CalculateYValue(float x, float z);
	float CalculateYValueWorld(float x, float z);
	void render(ID3D11DeviceContext *context, int instanceIndex = 0);
	void setColourMap(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* tex_colour);
	void getValueFromHeightMap(UINT8* Result, int i, int j, int width, int height, int texWidth, int texHeight, XMFLOAT3* P, XMFLOAT2* T);
	ExtendedVertexStruct* getHeightArray(){return vertices;};
	DirectX::XMUINT4 getColourMap(float x, float z);
	HRESULT init(ID3D11Device* device) { return S_OK; };
	HRESULT init(ID3D11Device *device, ID3D11DeviceContext* context,int _width,int _height, ID3D11Texture2D*tex_height);
	~Terrain();
};

