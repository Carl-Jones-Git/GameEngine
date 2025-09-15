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