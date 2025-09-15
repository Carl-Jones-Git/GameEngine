#pragma once
#include <BaseModel.h>
#include <VertexStructures.h>



class Grid : public BaseModel {
protected:

	// Create the indices
	bool visible = true;
	UINT width, height;
	UINT numVert = 0;
	UINT numInd = 0;

public:
	Grid(UINT _width, UINT  _height, ID3D11Device *device, shared_ptr<Effect> _effect, shared_ptr<Material> _material = nullptr) : BaseModel(device, _effect, _material){ init(device, _width, _height); }
	~Grid();

	UINT getWidth(){return width;};
	UINT getHeight(){ return height; };
	UINT getNumInd(){ return numInd; };
	bool getVisible(){ return visible; };
	void setVisible(bool _visible){ visible = _visible; };
	void render(ID3D11DeviceContext *context, int instanceIndex = 0);
	HRESULT init(ID3D11Device *device, UINT width, UINT  height);
	HRESULT init(ID3D11Device *device){ return S_OK; };
};