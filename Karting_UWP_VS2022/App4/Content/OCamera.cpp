#include "pch.h"
#include "Camera.h"
//#include <iostream>
Camera::Camera(){};
Camera::Camera(ID3D11Device *device) {
	pos = DirectX::XMVectorSet(0, 0, -10, 1.0f);
	up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	lookAt = DirectX::XMVectorZero();
	projMatrix = DirectX::XMMatrixPerspectiveFovLH(0.25f*3.14, 1280.0 / 720.0, 0.5f, 10000.0f);
	initCBuffer(device);
}
Camera::Camera(ID3D11Device *device,DirectX::XMVECTOR init_pos, DirectX::XMVECTOR init_up, DirectX::XMVECTOR init_lookAt) {
	pos = init_pos;
	up = init_up;
	lookAt = init_lookAt;
	projMatrix = DirectX::XMMatrixPerspectiveFovLH(0.25f*3.14, 1280.0 / 720.0, 0.5f, 1000.0f);
	initCBuffer(device);
}
Camera::Camera(ID3D11Device *device, DirectX::XMVECTOR init_pos, DirectX::XMVECTOR init_up, DirectX::XMVECTOR init_lookAt, DirectX::XMMATRIX _projMatrix) {
	pos = init_pos;
	up = init_up;
	lookAt = init_lookAt;
	projMatrix =_projMatrix;
	initCBuffer(device);
}
void Camera::initCBuffer(ID3D11Device *device){
	cBufferCPU = (CBufferCamera*)_aligned_malloc(sizeof(CBufferCamera), 16);
	cBufferCPU->viewMatrix = getViewMatrix();
	cBufferCPU->projMatrix = getProjMatrix();
	XMStoreFloat4(&(cBufferCPU->eyePos), getPos());

	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));
	cbufferDesc.ByteWidth = sizeof(CBufferCamera);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferCPU;
	device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferGPU);
}

Camera::~Camera()
{
}
void Camera::update(ID3D11DeviceContext *context) {
	cBufferCPU->viewMatrix = getViewMatrix();
	cBufferCPU->projMatrix = getProjMatrix();
	XMStoreFloat4(&(cBufferCPU->eyePos), getPos());
	mapCbuffer(context, cBufferCPU, cBufferGPU, sizeof(CBufferCamera));
	context->PSSetConstantBuffers(1, 1, &cBufferGPU);
	context->VSSetConstantBuffers(1, 1, &cBufferGPU);
	context->GSSetConstantBuffers(1, 1, &cBufferGPU);// Attach CBufferLightGPU to register b2 for the Pixel shader.

}

ID3D11Buffer* Camera::getCBuffer() {
	return cBufferGPU;
}

//
// Accessor methods
//
float Camera::getYaw() {
	DirectX::XMVECTOR det;
	DirectX::XMMATRIX view = getViewMatrix();
	DirectX::XMMATRIX inverseView = XMMatrixInverse(&det, view);
	DirectX::XMFLOAT4X4 viewMatrix4X4, inverseView4X4;
	XMStoreFloat4x4(&viewMatrix4X4, view);
	XMStoreFloat4x4(&inverseView4X4, inverseView);
	// The axis basis vectors and camera position are stored inside the
	// position matrix in the 4 rows of the camera's world matrix.
	// To figure out the yaw/pitch of the camera, we just need the Z basis vector.
	DirectX::XMFLOAT3 zBasis;
	DirectX::XMStoreFloat3(&zBasis, inverseView.r[2]);
	float             cameraYawAngle;
	cameraYawAngle = atan2f(zBasis.x, zBasis.z);
	return cameraYawAngle;
}

float Camera::getPitch() {
	DirectX::XMVECTOR det;
	DirectX::XMMATRIX view = getViewMatrix();
	DirectX::XMMATRIX inverseView = XMMatrixInverse(&det, view);
	DirectX::XMFLOAT4X4 viewMatrix4X4, inverseView4X4;
	XMStoreFloat4x4(&viewMatrix4X4, view);
	XMStoreFloat4x4(&inverseView4X4, inverseView);
	// The axis basis vectors and camera position are stored inside the
	// position matrix in the 4 rows of the camera's world matrix.
	// To figure out the yaw/pitch of the camera, we just need the Z basis vector.
	DirectX::XMFLOAT3 zBasis;
	DirectX::XMStoreFloat3(&zBasis, inverseView.r[2]);
	float             cameraPitchAngle;
	float len = sqrtf(zBasis.z * zBasis.z + zBasis.x * zBasis.x);
	cameraPitchAngle = atan2f(zBasis.y, len);
	return cameraPitchAngle;
}

void Camera::setProjMatrix(DirectX::XMMATRIX setProjMat) {
	projMatrix = setProjMat;
}
void Camera::setLookAt(DirectX::XMVECTOR init_lookAt) {
	lookAt = init_lookAt;
}
void Camera::setPos(DirectX::XMVECTOR init_pos) {
	pos = init_pos;
}
void Camera::setUp(DirectX::XMVECTOR init_up) {
	up = init_up;
}
DirectX::XMMATRIX Camera::getViewMatrix() {
	return 		DirectX::XMMatrixLookAtLH(pos, lookAt, up);
}
DirectX::XMMATRIX Camera::getProjMatrix() {
	return 		projMatrix;
}
DirectX::XMVECTOR Camera::getPos() {
	return pos;
}
DirectX::XMVECTOR Camera::getLookAt() {
	return lookAt;
}
DirectX::XMVECTOR Camera::getUp() {
	return up;
}

