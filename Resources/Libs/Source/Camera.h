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
#include<CBufferStructures.h>
#include<Utils.h>
#include<Includes.h>
class Camera
{
protected:
	DirectX::XMVECTOR pos;
	DirectX::XMVECTOR up;
	DirectX::XMVECTOR lookAt;
	DirectX::XMMATRIX projMatrix;
	ID3D11Buffer					*cBufferGPU = nullptr;
	CBufferCamera					*cBufferCPU = nullptr;
	void Camera::initCBuffer(ID3D11Device *device);
public:
	Camera();
	Camera(ID3D11Device *device);
	Camera(ID3D11Device *device, DirectX::XMVECTOR init_pos, DirectX::XMVECTOR init_up, DirectX::XMVECTOR init_lookAt, DirectX::XMMATRIX _projMatrix=XMMatrixPerspectiveFovLH(0.4f * 3.14, 1920.0f / 1080.0f, 0.5f, 1000.0f) );
	~Camera();

	// Accessor methods
	void setProjMatrix(DirectX::XMMATRIX setProjMat);
	void setLookAt(DirectX::XMVECTOR init_lookAt);
	void setPos(DirectX::XMVECTOR init_pos);
	void setUp(DirectX::XMVECTOR init_up);
	DirectX::XMMATRIX getViewMatrix();
	DirectX::XMMATRIX getProjMatrix();
	DirectX::XMVECTOR getPos();
	DirectX::XMVECTOR getLookAt();
	DirectX::XMVECTOR getUp();
	ID3D11Buffer* getCBuffer();
	float getYaw();
	float getPitch();

	void update(ID3D11DeviceContext *context);
};

