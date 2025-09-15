#pragma once
#include <Utils.h>

class Effect
{
	// Pipeline Input Layout
	ID3D11InputLayout						*VSInputLayout = nullptr;
	
	// Pipeline Shaders
	ID3D11VertexShader						*VertexShader = nullptr;
	ID3D11PixelShader						*PixelShader = nullptr;
	ID3D11GeometryShader					*GeometryShader = nullptr;
	ID3D11HullShader						*HullShader = nullptr;
	ID3D11DomainShader						*DomainShader = nullptr;
	
	// Pipeline States
	ID3D11RasterizerState					*RasterizerState = nullptr;
	ID3D11DepthStencilState					*DepthStencilState = nullptr;
	ID3D11BlendState						*BlendState = nullptr;
	FLOAT			blendFactor[4];
	UINT			sampleMask;
	
public:
	// Setup pipeline for this effect
	void bindPipeline(ID3D11DeviceContext *context);
	
	// Initalise Default Pipeline States
	void initDefaultStates(ID3D11Device *device);
	
	// Assign pre-loaded shaders
	Effect(ID3D11Device *device, ID3D11VertexShader	*_VertexShader, ID3D11PixelShader *_PixelShader, ID3D11InputLayout *_VSInputLayout);
	
	//Load shaders given shader path
	Effect(ID3D11Device *device, const char *vertexShaderPath, const char *pixelShaderPath, const D3D11_INPUT_ELEMENT_DESC vertexDesc[], UINT numVertexElements);

	//Copy Constructor
	Effect(shared_ptr<Effect> _effect)
	{
		// Pipeline Input Layout
		VSInputLayout = _effect->VSInputLayout;
		if (VSInputLayout)
		VSInputLayout->AddRef();

		// Pipeline Shaders
		VertexShader = _effect->VertexShader;
		if (VertexShader)
		VertexShader->AddRef();

		PixelShader = _effect->PixelShader;
		if (PixelShader)
		PixelShader->AddRef();

		GeometryShader = _effect->GeometryShader;
		if(GeometryShader)
			GeometryShader->AddRef();

		HullShader = _effect->HullShader;
		if (HullShader)
			HullShader->AddRef();

		DomainShader = _effect->DomainShader;
		if (DomainShader)
			DomainShader->AddRef();

		// Pipeline States
		RasterizerState = _effect->RasterizerState;
		if (RasterizerState)
			RasterizerState->AddRef();

		DepthStencilState = _effect->DepthStencilState;
		if (DepthStencilState)
			DepthStencilState->AddRef();

		BlendState = _effect->BlendState;
		if (BlendState)
			BlendState->AddRef();
		blendFactor[0]= _effect->blendFactor[0];
		blendFactor[1] = _effect->blendFactor[1];
		blendFactor[2] = _effect->blendFactor[2];
		blendFactor[3] = _effect->blendFactor[3];
		sampleMask=_effect->sampleMask;
	};

	// Getter and setter methods
	ID3D11InputLayout		*getVSInputLayout(){ return VSInputLayout; };
	ID3D11VertexShader		*getVertexShader(){ return VertexShader; };
	ID3D11PixelShader		*getPixelShader(){ return PixelShader; };
	ID3D11GeometryShader	*getGeometryShader(){ return GeometryShader; };
	ID3D11RasterizerState	*getRasterizerState(){ return RasterizerState; };
	ID3D11DepthStencilState	*getDepthStencilState(){ return DepthStencilState; };
	ID3D11BlendState		*getBlendState(){ return BlendState; };

	void setPixelShader(ID3D11PixelShader	*_PixelShader){ PixelShader = _PixelShader; };
	void setGeometryShader(ID3D11GeometryShader	*_GeometryShader){ GeometryShader = _GeometryShader; };
	void setVertexShader(ID3D11VertexShader	*_VertexShader){ VertexShader = _VertexShader; };
	void setVSInputLayout(ID3D11InputLayout	*_VSInputLayout){ VSInputLayout = _VSInputLayout; };
	void setRasterizerState(ID3D11RasterizerState	*_RasterizerState){ RasterizerState = _RasterizerState; };
	void setDepthStencilState(ID3D11DepthStencilState	*_DepthStencilState){ DepthStencilState = _DepthStencilState; };
	void setBlendState(ID3D11BlendState	*_BlendState){ BlendState = _BlendState; };

	void setCullMode(ID3D11Device* device, D3D11_CULL_MODE cullMode)
	{
		D3D11_RASTERIZER_DESC rSDesc;
		RasterizerState->GetDesc(&rSDesc);//this just gets the default state description
		rSDesc.CullMode = cullMode;
		RasterizerState->Release();
		device->CreateRasterizerState(&rSDesc, &RasterizerState);
	}

	void setDepthWriteMask(ID3D11Device* device, D3D11_DEPTH_WRITE_MASK depthWriteMask)
	{
		D3D11_DEPTH_STENCIL_DESC dSDesc;
		DepthStencilState->GetDesc(&dSDesc);//this just gets the default state description
		dSDesc.DepthWriteMask = depthWriteMask;
		DepthStencilState->Release();
		device->CreateDepthStencilState( &dSDesc, &DepthStencilState );
	}

	void setDepthFunction(ID3D11Device* device, D3D11_COMPARISON_FUNC depthFunc)
	{
		D3D11_DEPTH_STENCIL_DESC dSDesc;
		DepthStencilState->GetDesc(&dSDesc);
		dSDesc.DepthFunc = depthFunc;
		DepthStencilState->Release();
		device->CreateDepthStencilState(&dSDesc, &DepthStencilState);
	}

	void setAlphaToCoverage(ID3D11Device* device, bool enable)
	{
		D3D11_BLEND_DESC bSDesc;
		BlendState->GetDesc(&bSDesc);//gets current state description
		//Set Alpha to Coverage
		bSDesc.AlphaToCoverageEnable = enable;
		if (enable)// Alpha Blending Off
			bSDesc.RenderTarget[0].BlendEnable = FALSE;
		BlendState->Release();
		device->CreateBlendState(&bSDesc, &BlendState);
	}
	void setAlphaBlendEnable(ID3D11Device* device, bool enable)
	{
		D3D11_BLEND_DESC bSDesc;
		BlendState->GetDesc(&bSDesc);//this just gets the default state description
		//Set Alpha Blending
		bSDesc.RenderTarget[0].BlendEnable = enable;
		bSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		if(enable)// Alpha to Coverage Off
			bSDesc.AlphaToCoverageEnable = FALSE;
		BlendState->Release();
		device->CreateBlendState(&bSDesc, &BlendState);
	}
	// Shader Creation Wrapper methods
	uint32_t Effect::CreateVertexShader(ID3D11Device *device, const char *filename, char **VSBytecode, ID3D11VertexShader **vertexShader);
	HRESULT Effect::CreatePixelShader(ID3D11Device *device, const char *filename, char **PSBytecode, ID3D11PixelShader **pixelShader);
	HRESULT Effect::CreateGeometryShader(ID3D11Device *device, const char *filename, char **GSBytecode, ID3D11GeometryShader **geometryShader);
	HRESULT Effect::CreateHullShader(ID3D11Device *device, const char *filename, char **GSBytecode, ID3D11HullShader **hullShader);
	HRESULT Effect::CreateDomainShader(ID3D11Device *device, const char *filename, char **GSBytecode, ID3D11DomainShader **domainShader);

	~Effect();
};

