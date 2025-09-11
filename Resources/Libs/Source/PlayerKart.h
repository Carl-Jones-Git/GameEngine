
#pragma once


#include <d3d11_2.h>
#include <Effect.h>
#include <Material.h>
#include "Model.h"
//#include <Scene.h>
#include <Effect.h>
#include <Kart.h>
#include <Texture.h>
#include <Xinput.h>
#include <FmodStudio\inc\fmod.hpp>

//#define PC_BUILD
#ifndef PC_BUILD
#include "..\Common\MoveLookController.h"
#endif

// Abstract base class to model mesh objects for rendering in DirectX
class PlayerKart : public Kart {

private:
	XINPUT_STATE controllerState;


public:
#ifndef PC_BUILD
	MoveLookController^ m_controller;
#endif
	PlayerKart(FMOD::System* _FMSystem, ID3D11Device* device, ID3D11DeviceContext* context, Terrain* ground, shared_ptr<Effect>  _perPixelEffect, shared_ptr<Texture> _texture) : Kart(_FMSystem, device,  context,ground,_perPixelEffect, _texture) { /*load(device, filename);*/ }
	~PlayerKart();

	void render(ID3D11DeviceContext *context,int camMode=1);
	void update(ID3D11DeviceContext* context, float stepSize);

	int camMode = 2;

};
