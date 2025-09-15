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

#include "Includes.h"
#include "FirstPersonCamera.h"
using namespace DirectX;

void FirstPersonCamera::move(float d)
{
	dir = lookAt - pos;
	pos += (dir*d);
	lookAt = pos + dir;
}

void FirstPersonCamera::turn(float theta)
{
	dir = lookAt - pos;
	XMVECTOR Q = XMQuaternionRotationAxis(up, theta);
	dir = XMVector3Rotate(dir, Q);
	lookAt = pos + dir;
}

void FirstPersonCamera::setHeight(float y)
{
	pos = XMVectorSetY(pos, y);
	lookAt = pos + dir;
}
void FirstPersonCamera::setPos(DirectX::XMVECTOR init_pos) {
	pos = init_pos;
	lookAt = pos + dir;
}
void FirstPersonCamera::elevate(float theta) 
{
	dir = lookAt - pos;
	XMVECTOR axis = XMVector3Cross(dir, up);
	XMVECTOR Q = XMQuaternionRotationAxis(axis, theta);
	dir=XMVector3Rotate(dir, Q);
	lookAt = pos + dir;
}
void  FirstPersonCamera::updateTopDown(ID3D11DeviceContext* context, DirectX::XMVECTOR pos, float height)
{
	setPos(XMVectorSet(XMVectorGetX(pos), height, XMVectorGetZ(pos), 1.0));
	setLookAt(XMVectorSet(XMVectorGetX(pos), 0, XMVectorGetZ(pos), 1));
	setUp(XMVectorSet(0, 0, 1, 1));
}
void FirstPersonCamera::updateFollow(ID3D11DeviceContext* context,XMVECTOR pos, XMVECTOR quart, float currentWeightDIR,float currentWeightPOS, XMVECTOR desiredCamOffset, XMVECTOR desiredLookAtOffset)
{

	//Direction player is currently facing
	XMVECTOR  playerDir = DirectX::XMVector4Transform(XMVectorSet(0, 0, 1.0, 1), XMMatrixRotationQuaternion(quart));
	static XMVECTOR oldDir = playerDir;//should be old cam dir
	static XMVECTOR oldCamPos = getPos();
	
	//Weights are used to lerp between the old cam pos/dir and the new cam target pos/dir so the camera tends to lag behind the kart 
	//the lag is inversly proportional to the weight so for first person the weight is closer to 1.0 for third person views the weight is closer to 0.0
	XMVECTOR lerpedPlayerDir = XMVectorAdd(XMVectorScale(playerDir, currentWeightDIR), XMVectorScale(oldDir, 1.0f - currentWeightDIR));//Lerp Dir
	XMVECTOR  desiredCamPos = XMVectorAdd(pos+XMVectorSet(0, XMVectorGetY(desiredCamOffset), 0, 0), XMVectorScale(lerpedPlayerDir, XMVectorGetZ(desiredCamOffset)));
	XMVECTOR lerpedCamPos= XMVectorAdd(XMVectorScale(desiredCamPos, currentWeightPOS), XMVectorScale(oldCamPos, 1.0f - currentWeightPOS));//Lerp Pos
	setPos(lerpedCamPos);//Lerp Pos

	XMVECTOR lookAtPos = XMVectorAdd(pos + XMVectorSet(0, XMVectorGetY(desiredLookAtOffset), 0, 0), XMVectorScale(lerpedPlayerDir, XMVectorGetZ(desiredLookAtOffset)));
	setLookAt(lookAtPos);//
		
	setUp(XMVectorSet(0, 1, 0, 1));

	oldDir = lerpedPlayerDir;
	oldCamPos = getPos();
	
	update(context);
}