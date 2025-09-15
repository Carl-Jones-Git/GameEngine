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
#include "BaseModel.h"
#include "CBufferStructures.h"
#include "VertexStructures.h"
//#include "Camera.h"
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
	DirectX::XMUINT4 getMapColour(float x, float z);
	HRESULT init(ID3D11Device* device) { return S_OK; };
	HRESULT init(ID3D11Device *device, ID3D11DeviceContext* context,int _width,int _height, ID3D11Texture2D*tex_height);
	~Terrain();
};

