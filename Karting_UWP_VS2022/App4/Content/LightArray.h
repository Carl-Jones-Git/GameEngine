#pragma once
#include"CBufferStructures.h"
#include"Utils.h"

class LightArray
{
	CBufferLight* cBufferLightCPU = nullptr;
	ID3D11Buffer* cBufferLightGPU = nullptr;
	void initCBuffer(ID3D11Device *device);
	int numLights;
public:
	LightArray(ID3D11Device *device);
	LightArray(ID3D11Device *device, XMFLOAT4 _lightVec, XMFLOAT4 _lightAmbient, XMFLOAT4 _lightDiffuse, XMFLOAT4 _lightSpecular, XMFLOAT4 _lightAttenuation);
	LightArray(ID3D11Device *device, CBufferLight *_lights, int _numLights);
	void addLight(CBufferLight *_light) {/*to do*/};
	void setLight(CBufferLight *_light) {/*to do*/};
	void setLightVector(ID3D11DeviceContext *context, int i, XMFLOAT4 _lightVec) {cBufferLightCPU[i].lightVec = _lightVec; update(context);};
	void setLightAmbient(ID3D11DeviceContext *context, int i, XMFLOAT4 _lightAmbient) { cBufferLightCPU[i].lightAmbient = _lightAmbient; update(context); };
	void setLightDiffuse(ID3D11DeviceContext *context, int i, XMFLOAT4 _lightDiffuse) { cBufferLightCPU[i].lightDiffuse = _lightDiffuse; update(context); };
	void setLightSpecular(ID3D11DeviceContext *context, int i, XMFLOAT4 _lightSpecular) { cBufferLightCPU[i].lightSpecular = _lightSpecular; update(context); };
	void setLightAttenuation(ID3D11DeviceContext *context, int i, XMFLOAT4 _lightAttenuation) { cBufferLightCPU[i].lightAttenuation = _lightAttenuation; update(context); };

	CBufferLight* getLight(int i) {};
	
	void update(ID3D11DeviceContext *context);





	~LightArray();
};

