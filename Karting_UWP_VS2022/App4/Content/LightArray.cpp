#include "pch.h"
#include "LightArray.h"


LightArray::LightArray(ID3D11Device *device)
{
	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferLightCPU = (CBufferLight *)_aligned_malloc(sizeof(CBufferLight), 16);
	numLights = 1;
	// Fill out cBufferLightCPU
	cBufferLightCPU[0].lightVec = XMFLOAT4(-125.0, 100.0, 70.0, 1.0);
	cBufferLightCPU[0].lightAmbient = XMFLOAT4(0.1, 0.1, 0.1, 1.0);
	cBufferLightCPU[0].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[0].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[0].lightAttenuation = XMFLOAT4(1, 0.00, 0.0000, 1000.0);//const = 0.0; linear = 0.0; quad = 0.05; cutOff = 200.0;
	initCBuffer(device);
}
LightArray::LightArray(ID3D11Device *device, XMFLOAT4 _lightVec, XMFLOAT4 _lightAmbient, XMFLOAT4 _lightDiffuse, XMFLOAT4 _lightSpecular, XMFLOAT4 _lightAttenuation)
{
	numLights = 1;
	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferLightCPU = (CBufferLight *)_aligned_malloc(sizeof(CBufferLight), 16);

	// Fill out cBufferLightCPU
	cBufferLightCPU[0].lightVec = _lightVec;//MFLOAT4(-125.0, 100.0, 70.0, 1.0);
	cBufferLightCPU[0].lightAmbient = _lightAmbient;// XMFLOAT4(0.1, 0.1, 0.1, 1.0);
	cBufferLightCPU[0].lightDiffuse = _lightDiffuse;// XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[0].lightSpecular = _lightSpecular;// XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[0].lightAttenuation = _lightAttenuation;// XMFLOAT4(1, 0.00, 0.0000, 1000.0);//const = 0.0; linear = 0.0; quad = 0.05; cutOff = 200.0;
	initCBuffer(device);

}
LightArray::LightArray(ID3D11Device *device, CBufferLight *_lights,int _numLights)
{
	numLights = _numLights;
	cBufferLightCPU = (CBufferLight *)_aligned_malloc(sizeof(CBufferLight)*numLights, 16);

	for (int i = 0; i < numLights; i++)
	{
		cBufferLightCPU[i].lightVec = _lights[i].lightVec;//MFLOAT4(-125.0, 100.0, 70.0, 1.0);
		cBufferLightCPU[i].lightAmbient = _lights[i].lightAmbient;// XMFLOAT4(0.1, 0.1, 0.1, 1.0);
		cBufferLightCPU[i].lightDiffuse = _lights[i].lightDiffuse;// XMFLOAT4(1.0, 1.0, 1.0, 1.0);
		cBufferLightCPU[i].lightSpecular = _lights[i].lightSpecular;// XMFLOAT4(1.0, 1.0, 1.0, 1.0);
		cBufferLightCPU[i].lightAttenuation = _lights[i].lightAttenuation;// XMFLOAT4(1, 0.00, 0.0000, 1000.0);//const = 0.0; linear = 0.0; quad = 0.05; cutOff = 200.0;
	}


	initCBuffer(device);
}
void LightArray::initCBuffer(ID3D11Device *device) {
	// Create GPU resource memory copy of cBufferLight
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferLight)*numLights;
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferLightCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,
		&cBufferLightGPU);
}
void LightArray::update(ID3D11DeviceContext *context) {
	mapCbuffer(context, cBufferLightCPU, cBufferLightGPU, sizeof(CBufferLight)*numLights);
	context->PSSetConstantBuffers(2, 1, &cBufferLightGPU);// Attach CBufferLightGPU to register b2 for the Pixel shader.

}


LightArray::~LightArray()
{
}
