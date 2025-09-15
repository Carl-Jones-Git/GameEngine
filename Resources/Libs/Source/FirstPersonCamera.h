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

#include <DirectXMath.h>
#include <Camera.h>
#include <Includes.h>

class FirstPersonCamera : public Camera
{

private:
	DirectX::XMVECTOR dir;
	bool flying = true;

public:
	FirstPersonCamera(ID3D11Device *device) : Camera(device){ dir = XMVectorSet(-1, 0, 1, 1); };
	FirstPersonCamera(ID3D11Device *device, XMVECTOR init_pos, XMVECTOR init_up, XMVECTOR init_dir) :Camera(device, init_pos, init_up, XMVectorAdd(init_pos , init_dir)){ dir = init_dir; };
	FirstPersonCamera(ID3D11Device* device, XMVECTOR init_pos, XMVECTOR init_up, XMVECTOR init_dir, XMMATRIX _projMatrix) :Camera(device, init_pos, init_up, XMVectorAdd(init_pos, init_dir), _projMatrix) { dir = init_dir; };

	void updateFollow(ID3D11DeviceContext* context, DirectX::XMVECTOR pos, XMVECTOR quart=XMVectorSet(0, 0, 0, 0) , float currentWeightDIR=0.5, float  currentWeightPOS=0.7, XMVECTOR desiredCamOffset=XMVectorSet(0, 1, -0.6, 1), XMVECTOR desiredLookAtOffset=XMVectorSet(0,1,6,1));
	void updateTopDown(ID3D11DeviceContext* context, DirectX::XMVECTOR pos,float height=15);

	~FirstPersonCamera() {};
	void setHeight(float y);
	void setPos(DirectX::XMVECTOR init_pos);
	void move(float d);
	void turn(float d);
	void elevate(float d);
	bool getFlying(){ return flying; };
	void setFlying(bool _flying){ flying = _flying; };
	bool toggleFlying(){ flying = !flying; return flying; };
};
