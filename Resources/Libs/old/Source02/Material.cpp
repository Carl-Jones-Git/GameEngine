#include "stdafx.h"
#include "Material.h"
Material::Material()
{

}

Material::Material(ID3D11Device* device)
{
	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferMaterialCPU = (CBufferMaterial*)_aligned_malloc(sizeof(CBufferMaterial), 16);
	cBufferMaterialCPU->emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	cBufferMaterialCPU->ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cBufferMaterialCPU->diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cBufferMaterialCPU->specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cBufferMaterialCPU->useage = USE_MATERIAL_ALL;
	init(device);
}

Material::Material(ID3D11Device* device, XMFLOAT4 _colour,INT use)
{
	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferMaterialCPU = (CBufferMaterial*)_aligned_malloc(sizeof(CBufferMaterial), 16);
	cBufferMaterialCPU->emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	cBufferMaterialCPU->ambient = _colour;
	cBufferMaterialCPU->diffuse = _colour;
	cBufferMaterialCPU->specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	cBufferMaterialCPU->useage = use;
	init(device);
}


Material::Material(ID3D11Device* device, shared_ptr <Material>_material)
{
	// Add a CBuffer to store light properties - you might consider creating a Light Class to manage this CBuffer
// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferLight
	cBufferMaterialCPU = (CBufferMaterial*)_aligned_malloc(sizeof(CBufferMaterial), 16);
	cBufferMaterialCPU->emissive = _material->cBufferMaterialCPU->emissive;
	cBufferMaterialCPU->ambient = _material->cBufferMaterialCPU->ambient;
	cBufferMaterialCPU->diffuse = _material->cBufferMaterialCPU->diffuse;
	cBufferMaterialCPU->specular = _material->cBufferMaterialCPU->specular;
	cBufferMaterialCPU->useage = _material->cBufferMaterialCPU->useage;
	init(device);
}


void Material::update(ID3D11DeviceContext* context)
{

	// We dont strictly need to call map here as the GPU CBuffer was initialised from the CPU CBuffer at creation.
	// However if changes are made to the CPU CBuffer during update the we need to copy the data to the GPU CBuffer 
	// using the mapCbuffer helper function provided the in Util.h   

	mapCbuffer(context, cBufferMaterialCPU, cBufferMaterialGPU, sizeof(CBufferMaterial));
	context->VSSetConstantBuffers(4, 1, &cBufferMaterialGPU);// Attach CBufferLightGPU to register b2 for the vertex shader. Not strictly needed as our vertex shader doesnt require access to this CBuffer
	context->PSSetConstantBuffers(4, 1, &cBufferMaterialGPU);// Attach CBufferLightGPU to register b2 for the Pixel shader.
	for(int i=0;i< numTextures;i++)
		if(textureSRVArray[i]!=nullptr)
			context->PSSetShaderResources(0, numTextures, textureSRVArray);
}
void Material::init(ID3D11Device* device)
{
	for (int i = 0; i < MAX_TEXTURES; i++)
		textureSRVArray[i]=nullptr;

	// Create GPU resource memory copy of cBufferLight
	// fill out description (Note if we want to update the CBuffer we need  D3D11_CPU_ACCESS_WRITE)
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferMaterial);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferMaterialCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData,
		&cBufferMaterialGPU);
}
Material::~Material()
{
	cout << "Material destructor called" << endl;
	for (int i = 0; i < MAX_TEXTURES; i++)
		if(textureSRVArray[i])
			textureSRVArray[i]->Release();
	if (cBufferMaterialCPU)
		_aligned_free(cBufferMaterialCPU);
	if (cBufferMaterialGPU)
		cBufferMaterialGPU->Release();
}
