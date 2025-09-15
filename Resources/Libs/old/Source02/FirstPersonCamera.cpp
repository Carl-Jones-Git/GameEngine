#include "stdafx.h"
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
