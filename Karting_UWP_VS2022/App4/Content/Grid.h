#pragma once
#include "BaseModel.h"
#include "VertexStructures.h"



class Grid : public BaseModel {
protected:

	// Create the indices

	UINT width, height;
	UINT numVert = 0;
	UINT numInd = 0;

public:
	Grid(ID3D11Device *device, UINT _width, UINT  _height, Effect *_effect, Material *_materials[] = nullptr, int _numMaterials = 0, ID3D11ShaderResourceView **textures = nullptr, int numTextures = 0) : BaseModel(device, _effect, _materials, _numMaterials, textures, numTextures){ init(device, _width, _height); }

//	Grid(UINT width, UINT  height, ID3D11Device *device, Effect*_effect, ID3D11ShaderResourceView *tex_view, Material*_material);
	~Grid();

	UINT getWidth(){return width;};
	UINT getHeight(){ return height; };
	UINT getNumInd(){ return numInd; };

	void render(ID3D11DeviceContext *context);
	HRESULT init(ID3D11Device *device, UINT width, UINT  height);
	HRESULT init(ID3D11Device *device){ return S_OK; };
};