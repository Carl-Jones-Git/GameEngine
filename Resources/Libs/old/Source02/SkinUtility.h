#pragma once
#include <CBufferStructures.h>
#include "Model.h"
//class Model;
class Quad;
class Effect;
class SkinUtility
{

	float weight;
	D3D11_VIEWPORT							offScreenViewport; 
	ID3D11ShaderResourceView* intermedHSRV = nullptr;
	ID3D11RenderTargetView* intermedHRTV = nullptr;

	ID3D11ShaderResourceView* intermedSRV = nullptr;
	ID3D11RenderTargetView* intermedRTV = nullptr;

	ID3D11ShaderResourceView* irradTexSRV[6];
	ID3D11RenderTargetView* irradTexRTV[6];


	ID3D11DepthStencilView* depthStencilViewBlur = nullptr;
	ID3D11DeviceContext* context = nullptr;
	ID3D11Device* device = nullptr;
	Quad* screenQuad = nullptr;

	ID3D11BlendState* alphaOnBlendState = nullptr;
	ID3D11DepthStencilState* renderShadowDepthStencilState = nullptr;
	ID3D11VertexShader* screenQuadVS = nullptr;
	ID3D11PixelShader* skinPS = nullptr;
	ID3D11PixelShader* horizontalBlurPS = nullptr;
	ID3D11PixelShader* verticalBlurPS = nullptr;
	ID3D11PixelShader* textureCopyPS = nullptr;
	ID3D11PixelShader* depthCopyPS = nullptr;
	ID3D11VertexShader* perPixelLightingVS = nullptr;

	shared_ptr<Effect>  defaultEffect = nullptr;
	shared_ptr<Effect>  skinEffect = nullptr;
	shared_ptr<Effect>  texSpaceEffect = nullptr;
	CBufferTextSize* cBufferTextSizeCPU = nullptr;
	ID3D11Buffer* cBufferTextSizeGPU = nullptr;
	//ID3D11SamplerState* sampler = nullptr;
	void initCBuffer(ID3D11Device* device, int, int,float);
	void updateTextSize(ID3D11DeviceContext* context);
public:
	SkinUtility(ID3D11Device* deviceIn, ID3D11DeviceContext* contextIn, int blurWidth = 512, int blurHeight = 512,float _weight=1.0f);
	HRESULT setupBlurRenderTargets(ID3D11Device* deviceIn, int _blurWidth, int _blurHeight);
	void blurModel(Model* orb, ID3D11ShaderResourceView* depthSRV);
	void setGWeight(float _weight) { weight = _weight; };
	~SkinUtility();
};

