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

#include <d3d11_2.h>
#include <DirectXMath.h>
#include <string>
#include "VertexStructures.h"
#include "BaseModel.h"
class Effect;
class Camera;

class ShadowVolume : public BaseModel {

private:
	ID3D11Device			*device;	
	// Shadow  pipeline configuration
	shared_ptr<Effect> renderEffect = nullptr;
	ID3D11RasterizerState	*RSstateWire = nullptr; //Render wireframe volume for debugging 

	DWORD					numVerticesSdw,numFacesObj, dwNumVerticesObj,*edgesSdw = nullptr;
	uint32_t					*indicesObj = nullptr ;
	ID3D11InputLayout		*ShadowVertexLayout;
	ID3D11Buffer			*shadowBuffer;
	ID3D11Buffer			*stagingBuffer = nullptr;
	BasicVertexStruct		*verticesSdw = nullptr;
	BYTE					*verticesObj= nullptr;
	bool					objDoubleSided = false;

	void load(ID3D11Device *_device, ID3D11DeviceContext  *context, shared_ptr<Effect> _effect, XMVECTOR _pointLightVec, ID3D11Buffer *vertexBuffer, DWORD _vBSizeBytes, ID3D11Buffer  *_pIndices, DWORD _dwNumFace);
	void initialiseShadowBuffers(ID3D11Device *device, ID3D11DeviceContext  *context, ID3D11Buffer *vertexBuffer, ID3D11Buffer  *_pIndices);
	HRESULT init(ID3D11Device *device) { return S_OK; };
	void addEdge(DWORD* pEdges, DWORD& dwNumEdges, DWORD v0, DWORD v1);

	DWORD stride;

public:
	ShadowVolume(ID3D11Device *_device, ID3D11DeviceContext  *context,
		shared_ptr<Effect>  _renderEffect,
		XMVECTOR _pointLightVec, ID3D11Buffer *vertexBuffer, DWORD _vBSizeBytes, ID3D11Buffer  *_pIndices, DWORD _dwNumFace,
		shared_ptr<Material>				_material = nullptr)
		: BaseModel(_device, _renderEffect, _material)
	{
		load(_device,   context,_renderEffect,_pointLightVec,  vertexBuffer, _vBSizeBytes, _pIndices,_dwNumFace );
	}
	~ShadowVolume();

	void render(ID3D11DeviceContext *context);
	void renderWire(ID3D11DeviceContext *context);
	HRESULT buildShadowVolume(ID3D11DeviceContext  *context,  DirectX::XMVECTOR vLight);
	
};

